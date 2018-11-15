#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "orazio_shell_globals.h"
#include "orazio_shell_commands.h"
#include "orazio_print_packet.h"
#include "orazio_client.h"
#define MAX_COMPLETIONS 1024

struct OrazioClient* client=0;
int run=1;

PacketStatus op_result;

sem_t state_sem;
int state_sem_init=0;

sem_t param_sem;
int param_sem_init=0;


SystemStatusPacket system_status={
  .header.type=SYSTEM_STATUS_PACKET_ID,
  .header.size=sizeof(SystemStatusPacket)
};

SystemParamPacket system_params={
  .header.type=SYSTEM_PARAM_PACKET_ID,
  .header.size=sizeof(SystemParamPacket)
};


SonarStatusPacket sonar_status={
  .header.type=SONAR_STATUS_PACKET_ID,
  .header.size=sizeof(SonarStatusPacket)
};

SonarParamPacket sonar_params={
  .header.type=SONAR_PARAM_PACKET_ID,
  .header.size=sizeof(SonarParamPacket)
};

StringMessagePacket message={
  .header.type=MESSAGE_PACKET_ID
};

DifferentialDriveStatusPacket drive_status = {
  .header.type=DIFFERENTIAL_DRIVE_STATUS_PACKET_ID,
  .header.size=sizeof(DifferentialDriveStatusPacket)
};
  
DifferentialDriveParamPacket drive_params = {
  .header.type=DIFFERENTIAL_DRIVE_PARAM_PACKET_ID,
  .header.size=sizeof(DifferentialDriveParamPacket)
};
EndEpochPacket end_epoch = {
  .type=END_EPOCH_PACKET_ID,
  .size=sizeof(EndEpochPacket)
};

ResponsePacket response = {
  .header.type=RESPONSE_PACKET_ID,
  .header.size=sizeof(ResponsePacket)
};

ParamControlPacket param_control={
  {.type=PARAM_CONTROL_PACKET_ID,
   .size=sizeof(ParamControlPacket),
   .seq=0
  },
  .action=ParamRequest,
  .param_type=ParamSystem
};

JointParamPacket joint_params[NUM_JOINTS] = {
  {
    .header.header.type=JOINT_PARAM_PACKET_ID,
    .header.header.size=sizeof(JointParamPacket),
    .header.index=0
  },
  {
    .header.header.type=JOINT_PARAM_PACKET_ID,
    .header.header.size=sizeof(JointParamPacket),
    .header.index=1
  }
};

JointStatusPacket joint_status[NUM_JOINTS] = {
  {
    .header.header.type=JOINT_STATUS_PACKET_ID,
    .header.header.size=sizeof(JointStatusPacket),
    .header.index=0
  },
  {
    .header.header.type=JOINT_STATUS_PACKET_ID,
    .header.header.size=sizeof(JointStatusPacket),
    .header.index=1
  }
};


JointControlPacket joint_control[NUM_JOINTS]=
  {
    {
      {
        {.type=JOINT_CONTROL_PACKET_ID,
         .size=sizeof(JointControlPacket),
         .seq=0
        },
        .index=0
      },
      {.mode=0,
       .speed=0
      }
    },
    {
      {
        {.type=JOINT_CONTROL_PACKET_ID,
         .size=sizeof(JointControlPacket),
         .seq=0
        },
        .index=1
      },
      {.mode=0,
       .speed=0
      }
    }
  };

DifferentialDriveControlPacket drive_control={
  {.type=DIFFERENTIAL_DRIVE_CONTROL_PACKET_ID,
   .size=sizeof(DifferentialDriveControlPacket),
   .seq=0
  },
  .translational_velocity=0.f,
  .rotational_velocity= 0.f
};


#define DECLARE_JOINT_VARS(idx)                                         \
  RV(joint_params[idx], TypePacket, VarRead),                           \
    RV(joint_params[idx].header.header.seq, TypeUInt16, VarRead),       \
    RV(joint_params[idx].param.kp, TypeInt16,VarRead|VarWrite),         \
    RV(joint_params[idx].param.ki, TypeInt16,VarRead|VarWrite),         \
    RV(joint_params[idx].param.kd, TypeInt16,VarRead|VarWrite),         \
    RV(joint_params[idx].param.max_i, TypeInt16,VarRead|VarWrite),      \
    RV(joint_params[idx].param.max_pwm, TypeInt16,VarRead|VarWrite),    \
    RV(joint_params[idx].param.min_pwm, TypeInt16,VarRead|VarWrite),    \
    RV(joint_params[idx].param.max_speed, TypeInt16,VarRead|VarWrite),  \
    RV(joint_params[idx].param.slope, TypeInt16,VarRead|VarWrite),      \
    RV(joint_params[idx].param.h_bridge_type, TypeUInt8,VarRead|VarWrite), \
    RV(joint_params[idx].param.h_bridge_pins[0], TypeInt8,VarRead|VarWrite), \
    RV(joint_params[idx].param.h_bridge_pins[1], TypeInt8,VarRead|VarWrite), \
    RV(joint_params[idx].param.h_bridge_pins[2], TypeInt8,VarRead|VarWrite), \
    RV(joint_control[idx],TypePacket, VarRead),                         \
    RV(joint_control[idx].header.header.seq, TypeInt16, VarRead),       \
    RV(joint_control[idx].control.mode, TypeUInt8,VarRead|VarWrite),    \
    RV(joint_control[idx].control.speed, TypeInt16,VarRead|VarWrite),   \
    RV(joint_status[idx], TypePacket, VarRead),                         \
    RV(joint_status[idx].header.header.seq, TypeUInt16, VarRead),       \
    RV(joint_status[idx].info.mode, TypeInt8, VarRead),                 \
    RV(joint_status[idx].info.encoder_position, TypeUInt16, VarRead),   \
    RV(joint_status[idx].info.encoder_speed, TypeInt16, VarRead),       \
    RV(joint_status[idx].info.desired_speed, TypeInt16, VarRead),       \
    RV(joint_status[idx].info.pwm, TypeInt16, VarRead),                 \
    RV(joint_status[idx].info.sensed_current, TypeInt16, VarRead)



VEntry variable_entries[]={
  // system status
  RV(system_status, TypePacket, VarRead),
  RV(system_status.header.seq, TypeUInt16, VarRead),
  RV(system_status.rx_buffer_size, TypeUInt16, VarRead),
  RV(system_status.rx_packets, TypeUInt16, VarRead),
  RV(system_status.rx_packet_errors, TypeUInt16, VarRead),
  RV(system_status.tx_buffer_size, TypeUInt16, VarRead),
  RV(system_status.tx_packets, TypeUInt16, VarRead),
  RV(system_status.tx_packet_errors, TypeUInt16, VarRead),
  RV(system_status.battery_level, TypeUInt16, VarRead),
  RV(system_status.watchdog_count, TypeUInt16, VarRead),
  RV(system_status.idle_cycles, TypeUInt32, VarRead),
  RV(system_status.rx_seq, TypeUInt16, VarRead),

  //system params
  RV(system_params, TypePacket, VarRead),
  RV(system_params.header.seq, TypeUInt16, VarRead),
  RV(system_params.protocol_version, TypeInt32Hex,VarRead),
  RV(system_params.firmware_version, TypeInt32Hex,VarRead),
  RV(system_params.timer_period_ms, TypeUInt16,VarRead),
  RV(system_params.comm_speed, TypeInt32,VarRead),
  RV(system_params.comm_cycles, TypeUInt16,VarRead|VarWrite),
  RV(system_params.periodic_packet_mask, TypeUInt8,VarRead|VarWrite),
  RV(system_params.watchdog_cycles, TypeUInt16,VarRead|VarWrite),
  RV(system_params.num_joints, TypeUInt8,VarRead),
  

  DECLARE_JOINT_VARS(0),
  DECLARE_JOINT_VARS(1),

  // drive status
  RV(drive_status, TypePacket, VarRead),
  RV(drive_status.header.seq, TypeUInt16, VarRead),
  RV(drive_status.enabled, TypeUInt8, VarRead),
  RV(drive_status.odom_x, TypeFloat, VarRead),
  RV(drive_status.odom_y, TypeFloat, VarRead),
  RV(drive_status.odom_theta, TypeFloat, VarRead),
  RV(drive_status.translational_velocity_measured, TypeFloat, VarRead),
  RV(drive_status.translational_velocity_desired, TypeFloat, VarRead),
  RV(drive_status.translational_velocity_adjusted, TypeFloat, VarRead),
  RV(drive_status.rotational_velocity_measured, TypeFloat, VarRead),
  RV(drive_status.rotational_velocity_desired, TypeFloat, VarRead),
  RV(drive_status.rotational_velocity_adjusted, TypeFloat, VarRead),
  //drive params
  RV(drive_params, TypePacket, VarRead),
  RV(drive_params.header.seq, TypeUInt16, VarRead),
  RV(drive_params.ikr, TypeFloat,VarRead|VarWrite),
  RV(drive_params.ikl, TypeFloat,VarRead|VarWrite),
  RV(drive_params.baseline, TypeFloat,VarRead|VarWrite),
  RV(drive_params.right_joint_index, TypeUInt8,VarRead|VarWrite),
  RV(drive_params.left_joint_index,  TypeUInt8,VarRead|VarWrite),
  RV(drive_params.max_translational_velocity, TypeFloat,VarRead|VarWrite),
  RV(drive_params.max_translational_acceleration, TypeFloat,VarRead|VarWrite),
  RV(drive_params.max_translational_brake, TypeFloat,VarRead|VarWrite),
  RV(drive_params.max_rotational_velocity, TypeFloat,VarRead|VarWrite),
  RV(drive_params.max_rotational_acceleration, TypeFloat,VarRead|VarWrite),

  //param control
  RV(param_control,TypePacket, VarRead),
  RV(param_control.header.seq, TypeInt16, VarRead),
  RV(param_control.action, TypeUInt8,VarRead|VarWrite),
  RV(param_control.param_type, TypeUInt8,VarRead|VarWrite),


  //drive control
  RV(drive_control,TypePacket, VarRead),
  RV(drive_control.header.seq, TypeInt16, VarRead),
  RV(drive_control.translational_velocity, TypeFloat,VarRead|VarWrite),
  RV(drive_control.rotational_velocity, TypeFloat,VarRead|VarWrite),

  //sonar param
  RV(sonar_params,TypePacket, VarRead),
  RV(sonar_params.pattern[0], TypeUInt8, VarRead|VarWrite),
  RV(sonar_params.pattern[1], TypeUInt8, VarRead|VarWrite),
  RV(sonar_params.pattern[2], TypeUInt8, VarRead|VarWrite),
  RV(sonar_params.pattern[3], TypeUInt8, VarRead|VarWrite),
  RV(sonar_params.pattern[4], TypeUInt8, VarRead|VarWrite),
  RV(sonar_params.pattern[5], TypeUInt8, VarRead|VarWrite),
  RV(sonar_params.pattern[6], TypeUInt8, VarRead|VarWrite),
  RV(sonar_params.pattern[7], TypeUInt8, VarRead|VarWrite),

  //sonar status
  RV(sonar_status,TypePacket, VarRead),
  RV(sonar_status.ranges[0], TypeUInt16, VarRead),
  RV(sonar_status.ranges[1], TypeUInt16, VarRead),
  RV(sonar_status.ranges[2], TypeUInt16, VarRead),
  RV(sonar_status.ranges[3], TypeUInt16, VarRead),
  RV(sonar_status.ranges[4], TypeUInt16, VarRead),
  RV(sonar_status.ranges[5], TypeUInt16, VarRead),
  RV(sonar_status.ranges[6], TypeUInt16, VarRead),
  RV(sonar_status.ranges[7], TypeUInt16, VarRead),
  


  //message
  RV(message,TypePacket, VarRead),
  RV(message.message,TypeString, VarRead)

};


const int num_variables=sizeof(variable_entries)/sizeof(VEntry);

const char *param_names[]={
  "system_params",
  "joint_params",
  "joint_params[0]",
  "joint_params[1]",
  "drive_params",
  "sonar_params",
  0
};

const char *control_names[]={
  "param_control",
  "joint_control",
  "joint_control[0]",
  "joint_control[1]",
  "drive_control",
  0
};

int stringInSet(const char** set, const char* s){
  const char**base=set;
  while(*base){
    if (!strcmp(*base, s))
      return 1;
    ++base;
  }
  return 0;
}

int isParamPacket(const char* name){
  return stringInSet(param_names, name);
}

int isControlPacket(const char* name){
  return stringInSet(control_names, name);
}


VEntry* findVar(const char* name){
  int var_idx=0;
  while(var_idx<num_variables){
    if (! strcmp(variable_entries[var_idx].name, name))
      return &variable_entries[var_idx];
    var_idx++;
  }
  return 0;
}

int getVarsByPrefix(VEntry** entries, const char* prefix){
  int k=0;
  int l=strlen(prefix);
  for (int i=0; i<num_variables; ++i){
    VEntry* var=variable_entries+i;
    if (! strncmp(prefix, var->name, l)){
      entries[k]=var;
      ++k;
    }
  }
  entries[k]=0;
  return k;
}

PacketStatus refreshState(void){
  op_result=Success;
  op_result = OrazioClient_get(client, (PacketHeader*)&system_status);
  if (op_result!=Success) return op_result;

  op_result=OrazioClient_get(client, (PacketHeader*)&drive_status);
  if (op_result!=Success) return op_result;

  for (int i=0; i<NUM_JOINTS; ++i) {
    op_result=OrazioClient_get(client, (PacketHeader*)&joint_status[i]);
    if (op_result!=Success) return op_result;
  }
 
  op_result=OrazioClient_get(client, (PacketHeader*)&sonar_status);
  
  if (op_result!=Success) return op_result;
  
  op_result=OrazioClient_get(client, (PacketHeader*)&end_epoch);
  OrazioClient_get(client, (PacketHeader*)&response);
  
  return op_result;
}

PacketStatus refreshParams(ParamType type){
  op_result=Success;
  switch(type){
  case ParamSystem:
    return OrazioClient_get(client, (PacketHeader*)&system_params);
  case ParamJointsSingle:
    for (int i=0; i<NUM_JOINTS; ++i) {
      op_result=OrazioClient_get(client, (PacketHeader*)&joint_params[i]);
      if(op_result!=Success)
        return op_result;
    }
    return op_result;
  case ParamDrive:
    return OrazioClient_get(client, (PacketHeader*)&drive_params);
  case ParamSonar:
    return OrazioClient_get(client, (PacketHeader*)&sonar_params);
  default:
    return GenericError;
  }
}

PacketStatus refreshMessage(void){
  return OrazioClient_get(client, (PacketHeader*)&message);
}



/* shell line completion*/
int commonSubstring(char* dest, const char* src){
  int k=0;
  while(*dest && *src && *dest == *src)
    ++k, ++dest, ++src;
  *dest=0;
  return k;
}

char** completeCommand(const char* line, int start, int end){
  rl_attempted_completion_over=1;
  char** completions=(char**)malloc(sizeof(char*)*MAX_COMPLETIONS);
  int k=1;
  int cmd_idx=0;
  while(cmd_idx<num_commands){
    const char* name=commands[cmd_idx].name;
    completions[k]=strdup(name);
    if (!strncmp(line, name, end-start)){
      if (k==1)
	completions[0]=strdup(name);
      else {
	commonSubstring(completions[0],name);
      }
      ++k;
    }
    ++cmd_idx;
  }
  if (k==1) {
    free(completions);
    return 0;
  }
  completions[k]=0;
  return completions;
}

char** completeVar(const char* line, int start, int end) {
  rl_attempted_completion_over=1;
  char** completions=(char**)malloc(sizeof(char*)*MAX_COMPLETIONS);
  int k=1;
  int var_idx=0;
  while(var_idx<num_variables){
    const char* name=variable_entries[var_idx].name;
    completions[k]=strdup(name);
    if (!strncmp(line, name, end-start)){
      if (k==1)
	completions[0]=strdup(name);
      else {
	commonSubstring(completions[0],name);
      }
      ++k;
    }
    ++var_idx;
  }
  if (k==1) {
    free(completions);
    return 0;
  }
  completions[k]=0;
  return completions;
}


char** completeLine(const char* line, int start, int end){
  if (start==0)
    return completeCommand(line, start, end);
 return completeVar(line, start, end);
}

void Orazio_shellStart(void) {
  printf("shell started\n");
  Orazio_printPacketInit();
  rl_attempted_completion_function = completeLine;
  while (run) {
    char *buffer = readline("orazio> ");
    if (buffer) {
      char response[10000];
      executeCommand(response, buffer);
      if (*buffer)
	add_history(buffer);
      free(buffer);
    } else
      run=0;
  }
}
