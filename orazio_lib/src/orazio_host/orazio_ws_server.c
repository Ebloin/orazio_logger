#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libwebsockets.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "orazio_client.h"
#include "orazio_print_packet.h"
#include "orazio_shell_globals.h"
#include "orazio_shell_commands.h"

#define PATH_SIZE 1024
#define BUF_SIZE 10240
#define NUM_VARIABLES 100
#define MAX_CONNECTIONS 1024

/*variables for the websocket server*/
typedef enum {
  System=1,
  Joint0=2,
  Joint1=3,
  Drive=4,
  Sonar=5
} WindowMode;

typedef struct lws * WebSocketConnectionPtr;

typedef struct {
  char client_response[BUF_SIZE];
  char* client_response_begin;
  int client_response_length;
  WebSocketConnectionPtr connections[MAX_CONNECTIONS];
  pthread_t thread;
  volatile int run;
  int rate;
  int port;
  WindowMode mode;
  struct OrazioClient* client;
  char resource_path[PATH_SIZE];
  VEntry *system_status_vars[NUM_VARIABLES];
  VEntry *system_params_vars[NUM_VARIABLES];
  VEntry *joint_status_vars[NUM_JOINTS][NUM_VARIABLES];
  VEntry *joint_params_vars[NUM_JOINTS][NUM_VARIABLES];
  VEntry *joint_control_vars[NUM_JOINTS][NUM_VARIABLES];
  VEntry *drive_status_vars[NUM_VARIABLES];
  VEntry *drive_params_vars[NUM_VARIABLES];
  VEntry *drive_control_vars[NUM_VARIABLES];
  VEntry *sonar_status_vars[NUM_VARIABLES];
  VEntry *sonar_params_vars[NUM_VARIABLES];
  VEntry *response_vars[NUM_VARIABLES];
  VEntry *message_vars[NUM_VARIABLES];
} OrazioWSContext;

static OrazioWSContext* ws_ctx=0;

int findConnection(OrazioWSContext* ctx, WebSocketConnectionPtr conn){
  for (int i=0; i<MAX_CONNECTIONS; ++i)
    if (ctx->connections[i]==conn)
      return i;
  return -1;
}

int getFreeConnectionIdx(OrazioWSContext* ctx){
  for (int i=0; i<MAX_CONNECTIONS; ++i)
    if (!ctx->connections[i])
      return i;
  return -1;
}
int freeConnection(OrazioWSContext* ctx, WebSocketConnectionPtr conn){
  int idx=findConnection(ctx, conn);
  if (idx<0)
    return -1;
  ctx->connections[idx]=0;
  return 0;
}

void initConnections(OrazioWSContext* ctx){
  memset(ctx->connections, 0, sizeof(WebSocketConnectionPtr)*MAX_CONNECTIONS);
}

void printVarInit(OrazioWSContext* ctx){
  printf("system_status_vars: %d\n",getVarsByPrefix(ctx->system_status_vars, "system_status."));
  printf("system_params_vars: %d\n",getVarsByPrefix(ctx->system_params_vars, "system_params."));
  for (int i=0; i<NUM_JOINTS; ++i) {
    char buf[1024];
    sprintf(buf, "joint_status[%d].",i);
    printf("joint_status_vars[%d]: %d\n",i, getVarsByPrefix(ctx->joint_status_vars[i], buf));
    sprintf(buf, "joint_params[%d].",i);
    printf("joint_params_vars[%d]: %d\n",i, getVarsByPrefix(ctx->joint_params_vars[i], buf));
    sprintf(buf, "joint_control[%d].",i);
    printf("joint_control_vars[%d]: %d\n",i, getVarsByPrefix(ctx->joint_control_vars[i], buf));
  }
  printf("drive_status_vars: %d\n",getVarsByPrefix(ctx->drive_status_vars, "drive_status."));
  printf("drive_params_vars: %d\n",getVarsByPrefix(ctx->drive_params_vars, "drive_params."));
  printf("drive_control_vars: %d\n",getVarsByPrefix(ctx->drive_control_vars, "drive_control."));
  printf("sonar_status_vars: %d\n",getVarsByPrefix(ctx->sonar_status_vars, "sonar_status."));
  printf("sonar_params_vars: %d\n",getVarsByPrefix(ctx->sonar_params_vars, "sonar_params."));
  printf("message_vars: %d\n",getVarsByPrefix(ctx->message_vars, "message."));
  printf("response_vars: %d\n",getVarsByPrefix(ctx->response_vars, "response_vars."));
}

int printVarListHTML(char* buffer, VEntry** list){
  char* s=buffer;
  s+=sprintf(s, "<p> <table border=\"1\">\n");
  while(*list){
    VEntry* var=*list;
    s+=sprintf(s, "<tr><td valign=\"top\"> %s </td>\n", var->name);
    if(var->access_flags&VarRead){
      s+=sprintf(s, "<td style=\"min-width:80px\">");
      s+=VEntry_write(s,var);
      s+=sprintf(s, "</td>");
    }
    s+=sprintf(s, "</tr>\n");
    ++list;
  }
  s+=sprintf(s, "</table> </p>");
  return s-buffer;
}

int genVarInputHTML(OrazioWSContext* ctx, const char* filename, VEntry** list){
  char buffer[10240];
  char* s=buffer;
  s+=sprintf(s, "<p> <table border=\"3\">\n");
  while(*list){
    VEntry* var=*list;
    ++list;
    if (! (var->access_flags&VarWrite))
      continue;

    // bloody javascript
    char resource_name[1024];
    strcpy(resource_name,var->name);
    char* r=resource_name;
    while(*r) {
      switch (*r) {
      case '.':
      case '[':
      case ']':
        *r='_';
        break;
      default:;
      }
      ++r;
    }
    s+=sprintf(s, "<tr>\n");
    s+=sprintf(s,"   <td valign=\"top\"> %s </td>\n", var->name);
    s+=sprintf(s,"   <td> <input width=\"100px\" type=\"text\" id=\"%s\" /> </td>\n", resource_name);
    s+=sprintf(s,"   <td> <button type=\"button\" onclick=\"setVariable('%s')\"/> SET</td>\n", var->name);
    s+=sprintf(s, "</tr>\n");
  }
  s+=sprintf(s, "</table> </p>\n");
  FILE* f=fopen(filename, "w");
  if (f){
    fprintf(f,"%s", buffer);
    fclose(f);
  } else {
    printf("cannot open file [%s]\n",filename);
  }
  return s-buffer;
}

static int callback_http(struct lws *wsi,
                         enum lws_callback_reasons reason, void *user,
                         void *in, size_t len);

static int callback_input_command( /*struct libwebsocket_context * this_context,*/
                                  struct lws *wsi,
                                  enum lws_callback_reasons reason,
                                  void *user, void *in, size_t len);

static int callback_http(struct lws *wsi,
                         enum lws_callback_reasons reason,
                         void *user,
                         void *in, size_t len) {
  OrazioWSContext* ctx=ws_ctx;
  char *requested_uri = (char *) in;
  char* mime = (char*) "text/html";
  switch (reason) {
  case LWS_CALLBACK_CLIENT_WRITEABLE:
    printf("WS: connection established\n");
    break;
  case LWS_CALLBACK_HTTP: 
    requested_uri = (char *) in;
    printf("WS: requested URI: %s\n", requested_uri);
    if(strstr(requested_uri,"system")){
      ctx->mode=System;
    } else if(strstr(requested_uri,"joint0")){
      ctx->mode=Joint0;
    } else if(strstr(requested_uri,"joint1")){
      ctx->mode=Joint1;
    } else if(strstr(requested_uri,"drive")){
      ctx->mode=Drive;
    } else if(strstr(requested_uri,"sonar")){
      ctx->mode=Sonar;
    } 
    printf("mode %d\n",ctx->mode);
    char resource_file[PATH_SIZE];
    strcpy(resource_file,ctx->resource_path);
    if (! strcmp(requested_uri,"/") ){
      strcat(resource_file,"/index.html");
    } else {
      strcat(resource_file,requested_uri);
    }
    printf("WS: serving %s\n",resource_file);
    lws_serve_http_file(wsi, resource_file, mime, 0, 0);
    return 1;
    break;
  default:;
  }
  return 0;
}


static int callback_input_command( /*struct libwebsocket_context * this_context,*/
                                  struct lws *wsi,
                                  enum lws_callback_reasons reason,
                                  void *user, void *in, size_t len) {
  OrazioWSContext* ctx=ws_ctx;
  int idx=0;
  switch (reason) {
  case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
    printf("connection established\n");
    idx=getFreeConnectionIdx(ctx);
    if (idx>=0)
      ctx->connections[idx]=wsi;
    break;

  case LWS_CALLBACK_SERVER_WRITEABLE: // just log message that someone is connecting
    lws_write(wsi, (unsigned char*) ctx->client_response, ctx->client_response_length, LWS_WRITE_TEXT);
    break;


  case LWS_CALLBACK_RECEIVE:  // the funny part
    {
      printf("WS: received data:[ %s]\n", (char*) in);
      char cmd_response[1024];
      executeCommand(cmd_response, (char*)in);
    }
    break;

  case LWS_CALLBACK_CLOSED: { // the funny part
    printf("connection closed \n");
    freeConnection(ctx, wsi);
    break;

  }
  default:
    break;
  }
    
  return 0;
}

void* _websocketFn(void* args){
  OrazioWSContext* ctx=(OrazioWSContext*) args;
  ws_ctx=ctx;
  // server url will be http://localhost:9000
  int port = ctx->port;
  const char *interface = NULL;
  // we're not using ssl
  //const char *cert_path = NULL;
  //const char *key_path = NULL;
  // no special options
    
  struct lws_context_creation_info info;

  memset(&info, 0, sizeof info);
  info.port = port;
  info.iface = interface;


  struct lws_protocols protocols[] = {
    /* first protocol must always be HTTP handler */
    {
      .name="http-only",   // name
      .callback=callback_http, // callback
      .per_session_data_size=0,
      .rx_buffer_size=0,
      .user=ctx
      // per_session_data_size
    },
    {
      .name="orazio-robot-protocol",   // name
      .callback=callback_input_command, // callback
      .per_session_data_size=0,
      .rx_buffer_size=0,
      .user=ctx
    },
    {
      .name=NULL,
      .callback=NULL,
      .per_session_data_size=0,   /* End of list */
      .rx_buffer_size=0,
      .user=ctx
    }
  };

  
  info.protocols = protocols;
  //info.extensions = lws_get_internal_extensions();
  info.ssl_cert_filepath = NULL;
  info.ssl_private_key_filepath = NULL;
  info.gid = -1;
  info.uid = -1;

  struct lws_context *context = lws_create_context(&info);    
  if (context == NULL) {
    fprintf(stderr, "libwebsocket init failed, no websocket server active\n");
    return 0;
  }
  int count=0;
  ctx->run=1;
  while (ctx->run) {
    char response_string[BUF_SIZE];
    char system_status_string[BUF_SIZE];
    char joint_status_string[NUM_JOINTS][BUF_SIZE];
    char drive_status_string[BUF_SIZE];
    char system_params_string[BUF_SIZE];
    char joint_params_string[NUM_JOINTS][BUF_SIZE];
    char drive_params_string[BUF_SIZE];
    char sonar_status_string[BUF_SIZE];
    char sonar_params_string[BUF_SIZE];
    
    char message_string[BUF_SIZE];
    printVarListHTML(response_string,ctx->response_vars);
    printVarListHTML(system_status_string,ctx->system_status_vars);
    printVarListHTML(system_params_string,ctx->system_params_vars);
    for (int i=0; i<NUM_JOINTS; ++i) {
      printVarListHTML(joint_status_string[i],ctx->joint_status_vars[i]);
      printVarListHTML(joint_params_string[i],ctx->joint_params_vars[i]);
    }
    printVarListHTML(drive_status_string,ctx->drive_status_vars);
    printVarListHTML(drive_params_string,ctx->drive_params_vars);
    printVarListHTML(sonar_status_string,ctx->sonar_status_vars);
    printVarListHTML(sonar_params_string,ctx->sonar_params_vars);
    printVarListHTML(message_string,ctx->message_vars);
    char buffer[102400];
    char* bend=buffer;
    const char* mode_name=0;
    const char* status_string=0;
    const char* param_string=0;
    switch(ctx->mode) {
    case System:
      mode_name="System";
      status_string=system_status_string;
      param_string=system_params_string;
      break;
    case Joint0:
      mode_name="Joint0";
      status_string=joint_status_string[0];
      param_string=joint_params_string[0];
      break;
    case Joint1:
      mode_name="Joint1";
      status_string=joint_status_string[1];
      param_string=joint_params_string[1];
      break;
    case Drive:
      mode_name="Drive";
      status_string=drive_status_string;
      param_string=drive_params_string;
      break;
    case Sonar :
      mode_name="Sonar";
      status_string=sonar_status_string;
      param_string=sonar_params_string;
      break;
    default:;
    }
    if (mode_name) {
      bend+=sprintf(bend, "<p> <table border=\"0\">");
      bend+=sprintf(bend, "<tr>\n");
      bend+=sprintf(bend, "<th> %s Status  </th>\n", mode_name);
      bend+=sprintf(bend, "<th> %s Params  </th>\n", mode_name);
      bend+=sprintf(bend, "</tr>\n");
      bend+=sprintf(bend, "<tr>\n");
      bend+=sprintf(bend, "<td valign=\"top\"> %s </td>\n", status_string);
      bend+=sprintf(bend, "<td valign=\"top\"> %s </td>\n", param_string);
      bend+=sprintf(bend, "</tr>\n");
      bend+=sprintf(bend, "</table> </p>\n");
      bend+=sprintf(bend, "<p> %s </p>\n", response_string);
      bend+=sprintf(bend, "<p> %s </p>\n", message_string);
    
      ctx->client_response_length=bend-buffer+1;
      memcpy(ctx->client_response_begin, buffer, ctx->client_response_length);
      lws_callback_on_writable_all_protocol(context, &protocols[1]);
    }
    lws_service(context, 10);
    usleep(1000000/ctx->rate);
    count++; 
  }
  lws_context_destroy(context);
  return 0;
}

OrazioWSContext* OrazioWebsocketServer_start(struct OrazioClient* client,
                                             int port,
                                             char* resource_path,
                                             int rate){
  OrazioWSContext* context=(OrazioWSContext*)malloc(sizeof(OrazioWSContext));
  context->port=port;
  context->client=client;
  context->client_response_begin=context->client_response+LWS_PRE;
  context->client_response_length=0;
  context->rate=rate;
  strcpy(context->resource_path, resource_path);
  printVarInit(context);
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&context->thread, &attr, _websocketFn, context);
  return context;
}

void OrazioWebsocketServer_stop(OrazioWSContext* context){
  context->run=0;
  void* retval;
  pthread_join(context->thread, &retval);
  //free(context);
}

void OrazioWebsocketServer_genHtml(char* resource_path){
  OrazioWSContext context;
  printVarInit(&context);
  char filename[PATH_SIZE];
  sprintf(filename, "%s/%s",resource_path, "system_params.html");
  printf("filename: [%s]\n", filename);
  genVarInputHTML(&context, filename, context.system_params_vars);
  
  sprintf(filename, "%s/%s",resource_path, "joint0_params.html");
  genVarInputHTML(&context, filename, context.joint_params_vars[0]);

  sprintf(filename, "%s/%s",resource_path, "joint0_control.html");
  genVarInputHTML(&context, filename, context.joint_control_vars[0]);

  sprintf(filename, "%s/%s",resource_path, "joint1_params.html");
  genVarInputHTML(&context, filename, context.joint_params_vars[1]);

  sprintf(filename, "%s/%s",resource_path, "joint1_control.html");
  genVarInputHTML(&context, filename, context.joint_control_vars[1]);

  sprintf(filename, "%s/%s",resource_path, "drive_params.html");
  genVarInputHTML(&context, filename, context.drive_params_vars);

  sprintf(filename, "%s/%s",resource_path, "drive_control.html");
  genVarInputHTML(&context, filename, context.drive_control_vars);

  sprintf(filename, "%s/%s",resource_path, "sonar_params.html");
  printf("filename: [%s]\n", filename);
  genVarInputHTML(&context, filename, context.sonar_params_vars);

}
