#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libwebsockets.h>
#include "orazio_client.h"
#include "orazio_print_packet.h"
#include "orazio_shell_globals.h"
#include "orazio_shell_commands.h"
#include "orazio_ws_server.h"

#define PATH_SIZE 1024

const char *banner[]={
  "orazio",
  "generaized client for orazio",
  "usage:"
  "  $> orazio_robot_websocket_server <parameters>",
  "starts a web server on localhost:9000",
  "parameters: ",
  "-serial-device <string>: the serial port (default /dev/orazio)",
  "-resource-path <string>:  the path containing the html ",
  "        files that will be served by embedded http server (default PWD)",
  "        resource path should be set to the html folder of this repo",
  "-gen    : generates the updated html stubs for the current packets",
  "        the output will be written in resource-path. After generation run the script",
  "        gen_html.sh from the folder",
  "-rate <int>: web refresh rate [Hz] (default 20 Hz)",
  "        use low rate (e.g., 1 Hz) for wireless connection",
  0
};

void printBanner(){
  const char*const* line=banner;
  while (*line) {
    printf("%s\n",*line);
    line++;
  }
}

void* _runnerFn(void* args_){
  while(run){
    OrazioClient_sync(client,1);
    refreshMessage();
    refreshState();
    // if we wait for the state we warn the listener
    if (state_sem_init)
      sem_post(&state_sem);
    // if we have a param operation, we issue it and warn the listener
    if (param_sem_init){
      op_result=OrazioClient_sendPacket(client, (PacketHeader*)&param_control, 100);
      if (op_result==Success)
        refreshParams((ParamType)param_control.param_type);
      sem_post(&param_sem);
    }
  }
  return 0;
}

int main(int argc, char** argv){
  int c=1;
  char resource_path[PATH_SIZE];
  resource_path[0]=0;
  const char* device=0;
  uint32_t baudrate = 115200;
  int rate=20;
  while (c<argc) {
    if (! strcmp(argv[c],"-resource-path")) {
      c++;
      strcpy(resource_path, argv[c]);
    } else if (! strcmp(argv[c], "-serial-device") ) {
      c++;
      device=argv[c];
    } else if (! strcmp(argv[c], "-rate") ) { 
      c++;
      rate=atoi(argv[c]);
    } else if (! strcmp(argv[c], "-gen") ) {
      if (! strlen(resource_path))
        strcpy(resource_path,".");
      printf("generating stubs in folder [%s]\n", resource_path);
      OrazioWebsocketServer_genHtml(resource_path);
      return 0;
    } else if (! strcmp(argv[c], "-baud") ) { 
      c++;
      baudrate=atoi(argv[c]);
      rate=atoi(argv[c]);
    } else if (! strcmp(argv[c], "-h") || ! strcmp(argv[c], "-help")) {
      printBanner();
      return 0;
    }
    c++;
  }

  printf("running with parameters\n");;
  printf(" resource_path: %s\n", resource_path);
  printf(" serial_device: %s\n", device);
  printf(" baud: %d\n", baudrate);
  printf(" rate: %d Hz\n", rate);
  
  client=OrazioClient_init(device, baudrate);
  
  if (! client) {
    printf("Failed\n");
    exit(-1);
  }
  printf("Success\n");
  
  printf("Synching");
  int sync_cycles=50;
  for (int i=0; i<sync_cycles; ++i){
    OrazioClient_sync(client,1);
    printf(".");
    fflush(stdout);
  }
  printf(" Done\n");
  OrazioClient_readConfiguration(client, 100);
  
  printf("Looping\n");
  refreshParams(ParamSystem);
  refreshParams(ParamJointsSingle);
  refreshParams(ParamDrive);
  refreshParams(ParamSonar);
 
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  pthread_t runner_thread;
  pthread_create(&runner_thread, &attr, _runnerFn, 0);

  struct OrazioWSContext* web_server=0;

  if (strlen(resource_path)) {
    web_server=OrazioWebsocketServer_start(client,
                                           9000,
                                           resource_path,
                                           rate);
  }
  
  // we start the shell and wait here
  Orazio_shellStart();
  
  void* runner_result;
  pthread_join(runner_thread, &runner_result);
  if (web_server)
    OrazioWebsocketServer_stop(web_server);
  
  OrazioClient_destroy(client);
}
