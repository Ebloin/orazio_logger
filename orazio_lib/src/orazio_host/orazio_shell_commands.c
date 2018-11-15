#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "orazio_shell_globals.h"
#include "orazio_shell_commands.h"
#include "orazio_client.h"

int printFn(char* r, char** args){
  const char* var_name=args[0];
  VEntry* var=findVar(var_name);
  if(! var){
    r+=sprintf(r, "print: unknown variable name [%s]\n", var_name);
    return -1;
  }
  VEntry_write(r, var);
  printf("\n");
  return 0;
}

int setFn(char* r, char** args){
  const char* var_name=args[0];
  VEntry* var=findVar(var_name);
  if(! var){
    r+=sprintf(r, "set: unknown variable [%s]\n", var_name);
    return -1;
  }
  VEntry_read(var, args[1]);
  r+=sprintf(r,"%s ", var_name);
  r+=VEntry_write(r, var);
  r+=sprintf(r, "\n");
  return 0;
}

int quitFn(char* r, char** args){
  run=0;
  return 0;
}

int sendFn(char* r, char** args){
  const char* var_name=args[0];
  VEntry* var=findVar(var_name);
  if(! var){
    r+=sprintf(r, "send: unknown variable [%s]\n", var_name);
    return -1;
  }
  if (! isParamPacket(var_name) && ! isControlPacket(var_name)){
    r+=printf(r, "send: sending of variable [%s] not allowed\n", var_name);
  }
  if (var->type==TypePacket){
    return OrazioClient_sendPacket(client,(PacketHeader*) var->ptr, 0); 
  } else {
    r+=printf("send: allowed only for full packets\n");
    return -1;
  }
}

int paramFn(char* r, char* cmd_name, char** args, ParamAction action){
  const char* var_name=args[0];
  if (! isParamPacket(var_name)){
    r+=sprintf(r, "%s: only parameters can be loaded\n", cmd_name);
    return -1;
  }
  ParamControlPacket query={
    {
      .type=PARAM_CONTROL_PACKET_ID,
      .size=sizeof(ParamControlPacket),
      .seq=0
    },
    .action=action,
    .index=0
  };
  
  if(! strcmp(var_name,"system_params")){
    query.param_type=ParamSystem;
  } else if(! strcmp(var_name,"joint_params[0]")){
    query.param_type=ParamJointsSingle;
    query.index=0;
  } else if(! strcmp(var_name,"joint_params[1]")){
    query.param_type=ParamJointsSingle;
    query.index=1;
  } else if(! strcmp(var_name,"drive_params")){
    query.param_type=ParamDrive;
  } else if(! strcmp(var_name,"sonar_params")){
    query.param_type=ParamSonar;
  } else {
    r+=sprintf(r, "%s: cannot handle params of type [%s]\n", cmd_name, var_name);
    return -1;
  }
  param_control=query;
  sem_init(&param_sem, 0, 0);
  param_sem_init=1;
  sem_wait(&param_sem);
  param_sem_init=0;
  sem_destroy(&param_sem);
  return op_result;
}

int fetchFn(char* r, char** args){
  return paramFn(r, "fetch", args, ParamLoad);
}

int storeFn(char* r, char** args){
  return paramFn(r, "store", args, ParamSave);
}

int requestFn(char* r, char** args){
  return paramFn(r, "request", args, ParamRequest);
}

int saveConfigFn(char* r, char** args){
  FILE* f=fopen(args[0], "w");
  if (! f){
    sprintf(r, "cannot open file [%s]\n", args[0]);
    return -1;
  }
  char config_buffer[1024*10];
  config_buffer[0]=0;
  char* s=config_buffer;
  for (int i=0; i<num_variables; ++i){
    VEntry* var= variable_entries+i;
    if (var->type!=TypePacket && strstr(var->name,"params")){
      s+=sprintf(s, "%s ", var->name);
      s+=VEntry_write(s, var);
      s+=sprintf(s, "\n");
    }
  }
  fprintf(f, "%s",config_buffer);
  fclose(f);
  return 0;
}

int loadConfigFn(char* r, char** args){
  FILE* f=fopen(args[0], "r");
  if (! f){
    sprintf(r, "cannot open file [%s]\n", args[0]);
    return -1;
  }
  while(!feof(f)){
    char var_name[1024];
    char var_value[1024];
    int res = fscanf(f,"%s %s\n", var_name, var_value);
    printf("[%s]=[%s]\n", var_name, var_value);
    VEntry* var=findVar(var_name);
    if(! var){
      r+=sprintf(r, "set: unknown variable [%s]\n", var_name);
      return -1;
    }
    VEntry_read(var, var_value);
    r+=sprintf(r,"%s ", var_name);
    r+=VEntry_write(r, var);
    r+=sprintf(r, "\n");
  }
  return 0;
}
  
// only dump prints directly
int dumpFn(char* r, char** args){
  const char* var_name=args[0];
  VEntry* var=findVar(var_name);
  if(! var){
    return -1;
  }
  int dump=atoi(args[1]);
  if (dump>100)
    dump=100;
  char line[1024];
  sprintf(line, "dumping value of var %s for %d cycles\n", var->name, dump);
  printf("%s", line);
  sem_init(&state_sem, 0, 0);
  state_sem_init=1;
  for (int i=0; i<dump; ++i){
    sem_wait(&state_sem);
    char* s=line;
    s+=sprintf(s, "%s: ", var->name);
    s+=VEntry_write(s, var);
    s+=sprintf(s, "\n");
    printf("%s", line);
  }
  state_sem_init=0;
  sem_destroy(&state_sem);
  return 0;
}

Command commands[] = {
  {
    .name = "quit",
    .n_args = 0,
    .cmd_fn=quitFn,
    .help="usage: quit",
  },
  {
    .name = "print",
    .n_args = 1,
    .cmd_fn=printFn,
    .help="usage: print <variable or field>"
  },
  {
    .name = "set",
    .n_args = 2,
    .cmd_fn=setFn,
    .help="usage: set <variable or field> <value>"
  }, 
  {
    .name = "dump",
    .n_args = 2,
    .cmd_fn=dumpFn,
    .help="usage: dump <variable or field> <cycles>"
  },
  {
    .name = "send",
    .n_args = 1,
    .cmd_fn=sendFn,
    .help="usage: send <variable or field>"
  },
  {
    .name = "request",
    .n_args = 1,
    .cmd_fn=requestFn,
    .help="usage: request <parameter packet>"
  },
  {
    .name = "fetch",
    .n_args = 1,
    .cmd_fn=fetchFn,
    .help="usage: load <parameter packet>"
  },
  {
    .name = "store",
    .n_args = 1,
    .cmd_fn=storeFn,
    .help="usage: save <parameter packet>"
  },
  {
    .name = "save_config",
    .n_args = 1,
    .cmd_fn=saveConfigFn,
    .help="usage: save_config <filename>"
  },
  {
    .name = "load_config",
    .n_args = 1,
    .cmd_fn=loadConfigFn,
    .help="usage: load_config <filename>"
  }
};

const int num_commands=sizeof(commands)/sizeof(Command);

Command* findCommand(const char* name){
  int cmd_idx=0;
  while(cmd_idx<num_commands){
    if (! strcmp(commands[cmd_idx].name, name)){
      return &commands[cmd_idx];
    }
    cmd_idx++;
  }
  return 0;
}


int executeCommand(char* response, const char* line_){
  char line[1024];
  strcpy(line, line_);
  char* name=strtok(line," ");
  if (! name)
    return -1;
  Command* cmd=findCommand(name);
  if (! cmd){
    printf("ERROR: unknown command [%s]\n", name);
    return -1;
  }
  char* argptrs[10];
  int argno=0;
  while(argno<10 && (argptrs[argno] = strtok(NULL, " "))){
    ++argno;
  }
  int retval=0;
  response[0]=0;
  if (argno==cmd->n_args){
    if (cmd->cmd_fn){
      retval=(*cmd->cmd_fn)(response, argptrs);
      if(response[0])
        printf("%s\n",response);
      if (retval)
        printf("ERROR %d\n", retval);
      else
        printf("OK\n");
    } else {
      printf("ERROR: no handler for command\n");
    }
  } else {
    printf("ERROR: command not issued. Wrong number of arguments\n %s\n", cmd->help);
  }
  return retval;
}
