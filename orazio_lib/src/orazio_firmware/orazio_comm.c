#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "uart.h"
#include "deferred_packet_handler.h"
#include "orazio_packets.h"
#include "orazio_globals.h"
#include "orazio_comm.h"
#include "orazio_drive.h"
#include "orazio_param.h"
#include "orazio_sonar.h"
#include "orazio_joints.h"

// these are the possible incoming packets, each with a buffer of PACKETS_PER_TYPE_MAX
// see deferred_packet_handler.h

ParamControlPacket param_control_packet_buffer[PACKETS_PER_TYPE_MAX];
SystemParamPacket system_param_packet_buffer[PACKETS_PER_TYPE_MAX];
DifferentialDriveControlPacket differential_drive_control_packet_buffer[PACKETS_PER_TYPE_MAX];
DifferentialDriveParamPacket differential_drive_param_packet_buffer[PACKETS_PER_TYPE_MAX];
JointControlPacket joint_control_packet_buffer[PACKETS_PER_TYPE_MAX];
JointParamPacket joint_param_packet_buffer[PACKETS_PER_TYPE_MAX];

#ifdef _ORAZIO_USE_SONAR_
SonarParamPacket  sonar_param_packet_buffer[PACKETS_PER_TYPE_MAX];
#endif

// stuff used in this module
static struct UART* uart;

static DeferredPacketHandler packet_handler;
static uint16_t global_seq;
static ResponsePacket response={
  {.type=RESPONSE_PACKET_ID,
   .size=sizeof(ResponsePacket),
   .seq=0
  }
};

static EndEpochPacket end_epoch = {
  .type=END_EPOCH_PACKET_ID,
  .size=sizeof(EndEpochPacket),
  .seq=0
};


// this assembles a response packetv
void Orazio_sendResponse(PacketHeader* p, PacketStatus result){
  response.p_seq=p->seq;
  response.p_type=p->type;
  response.p_result=result;
  Orazio_sendPacket((PacketHeader*)&response);
}

#define DISABLE_DRIVE      0x01// if set, drive is disable upon receival
#define ECHO               0x02// if set, the copied packet is sent back
#define COPY               0x04// if set the packet is copied in a variable

typedef struct {
  uint8_t action_flags;   // this is an OR of the flags above
  void* dest;             // destination buffer OR, if max_index>0 an array of dest_buffer ptrs
  PacketStatus (*post_copy_fn)(void);// function that is invoked after the packet is received
  int8_t max_index;       // max index: 0 in case of non array vars
} PacketHandlerArgs;

static PacketHandlerArgs system_param_args = {
  .action_flags=COPY|ECHO,
  .dest=&system_params,
  // when changing the eystem params
  // also the timings for
  // the differential drive controller change
  .post_copy_fn=Orazio_driveInit, 
  .max_index=-1
};


static PacketHandlerArgs drive_control_args = {
  .action_flags=COPY,
  .dest=&drive_control,
  .post_copy_fn=0,
  .max_index=-1
};

static PacketHandlerArgs drive_param_args = {
  .action_flags=COPY|ECHO,
  .dest=&drive_params,
  .post_copy_fn=Orazio_driveInit,
  .max_index=-1
};

#ifdef _ORAZIO_USE_SONAR_
static PacketHandlerArgs sonar_param_args = {
  .action_flags=COPY|ECHO,
  .dest=&sonar_params,
  .post_copy_fn=Orazio_sonarInit,
  .max_index=-1
};
#endif

static PacketHandlerArgs joint_control_args = {
  .action_flags=COPY|DISABLE_DRIVE,
  .dest=joint_control,
  .post_copy_fn=0,
  .max_index=NUM_JOINTS
};

static PacketHandlerArgs joint_param_args = {
  .action_flags=COPY|ECHO,
  .dest=joint_params,
  .post_copy_fn=Orazio_jointsInit,
  .max_index=NUM_JOINTS
};


static PacketStatus Orazio_handlePacket(PacketHeader* p, void* args_) {
  PacketHandlerArgs* args=(PacketHandlerArgs*)args_;
  system_status.rx_seq=p->seq;
  uint8_t* dest=(uint8_t*) args->dest; // destination
  int8_t idx=-1;                // index
  PacketStatus r=Success;       // response;

  
  if (args->max_index>=0) {
    PacketIndexed* p_idx=(PacketIndexed*) p;
    idx=p_idx->index;
    if (idx >= args->max_index) {
      Orazio_sendResponse(p,GenericError);
      return GenericError;
    }
    if (args->action_flags&COPY) {
      dest+=idx*p->size;
    }
  }
  
  // here in dest, if any we have the destination
  // in idx we have the index
  
  if (args->action_flags&DISABLE_DRIVE)
    drive_status.enabled=0;

  // we deal with the copy
  if ( (args->action_flags&COPY) && dest){
    memcpy(dest, p, p->size);
    // id we need to do something after setting, we do
    if (args->post_copy_fn){         
      r = (*args->post_copy_fn)();
    }
  }
  
  if (args->action_flags&ECHO)
    Orazio_sendPacket((PacketHeader*)dest);

  Orazio_sendResponse(p,r);
  return Success;

}

static PacketStatus Orazio_handleParamCtl(PacketHeader* p, void* args) {
  PacketStatus r=Success;       // response;
  PacketHeader* target=0;
  ParamControlPacket* ctl=(ParamControlPacket*)p;
  system_status.rx_seq=ctl->header.seq;
  switch(ctl->param_type){
  case ParamSystem:
    target=(PacketHeader*)&system_params;
    break;
  case ParamJointsSingle:
    target=(PacketHeader*)&(joint_params[ctl->index]);
    break;
  case ParamDrive:
    target=(PacketHeader*)&drive_params;
    break;
#ifdef _ORAZIO_USE_SONAR_
  case ParamSonar:
    target=(PacketHeader*)&sonar_params;
    break;
#endif
  default:
    target=0;
    r=GenericError;
  }
  switch(ctl->action){
  case ParamRequest:
    r=Success;
    break;
  case ParamLoad:
    r=Orazio_paramLoad(ctl->param_type, ctl->index);
    break;
  case ParamSave:
    r=Orazio_paramSave(ctl->param_type, ctl->index);
    break;
  default:
    r=GenericError;
  }
  if (target){
    Orazio_sendPacket(target);
  }
  Orazio_sendResponse(p,r);
  return r;
}


void Orazio_initializePackets(void) {
  DeferredPacketHandler_installPacket(&packet_handler,
                                      PARAM_CONTROL_PACKET_ID,
                                      sizeof(ParamControlPacket),
                                      param_control_packet_buffer,
                                      PACKETS_PER_TYPE_MAX,
                                      Orazio_handleParamCtl,
                                      0);

  DeferredPacketHandler_installPacket(&packet_handler,
                                      SYSTEM_PARAM_PACKET_ID,
                                      sizeof(SystemParamPacket),
                                      system_param_packet_buffer,
                                      PACKETS_PER_TYPE_MAX,
                                      Orazio_handlePacket,
                                      &system_param_args);

  DeferredPacketHandler_installPacket(&packet_handler,
                                      JOINT_CONTROL_PACKET_ID,
                                      sizeof(JointControlPacket),
                                      joint_control_packet_buffer,
                                      PACKETS_PER_TYPE_MAX,
                                      Orazio_handlePacket,
                                      &joint_control_args);
  
  DeferredPacketHandler_installPacket(&packet_handler,
                                      JOINT_PARAM_PACKET_ID,
                                      sizeof(JointParamPacket),
                                      joint_param_packet_buffer,
                                      PACKETS_PER_TYPE_MAX,
                                      Orazio_handlePacket,
                                      &joint_param_args);

  DeferredPacketHandler_installPacket(&packet_handler,
                                      DIFFERENTIAL_DRIVE_CONTROL_PACKET_ID,
                                      sizeof(DifferentialDriveControlPacket),
                                      differential_drive_control_packet_buffer,
                                      PACKETS_PER_TYPE_MAX,
                                      Orazio_handlePacket,
                                      &drive_control_args);

  DeferredPacketHandler_installPacket(&packet_handler,
                                      DIFFERENTIAL_DRIVE_PARAM_PACKET_ID,
                                      sizeof(DifferentialDriveParamPacket),
                                      differential_drive_param_packet_buffer,
                                      PACKETS_PER_TYPE_MAX,
                                      Orazio_handlePacket,
                                      &drive_param_args);

#ifdef _ORAZIO_USE_SONAR_
  DeferredPacketHandler_installPacket(&packet_handler,
                                      SONAR_PARAM_PACKET_ID,
                                      sizeof(SonarParamPacket),
                                      sonar_param_packet_buffer,
                                      PACKETS_PER_TYPE_MAX,
                                      Orazio_handlePacket,
                                      &sonar_param_args);
#endif
}


void Orazio_flushInputBuffers(void) {
  system_status.rx_buffer_size=UART_rxBufferFull(uart);
  while (UART_rxBufferFull(uart)){
    uint8_t c=UART_getChar(uart);
    PacketStatus status=PacketHandler_rxByte(&packet_handler.base_handler, c);
    if(status<0){
      ++system_status.rx_packet_errors;
    }
    if(status==SyncChecksum)
      ++system_status.rx_packets;
  }
}

int Orazio_flushOutputBuffers(void){
  while (packet_handler.base_handler.tx_size)
    UART_putChar(uart, PacketHandler_txByte(&packet_handler.base_handler));
  system_status.tx_buffer_size=UART_txBufferFull(uart);
  return packet_handler.base_handler.tx_size;
}

PacketStatus Orazio_sendPacket(PacketHeader* p){
  p->seq=global_seq;
  PacketStatus status=PacketHandler_sendPacket(&packet_handler.base_handler, p);    
  if (status==Success)
    ++system_status.tx_packets;
  else
    ++system_status.tx_packet_errors;
  return status;
}

void Orazio_commInit(void){
  uart = UART_init("uart_0",system_params.comm_speed);
  global_seq=0;
  DeferredPacketHandler_initialize(&packet_handler);
  Orazio_initializePackets();
}


void Orazio_commHandle(void){
  Orazio_flushInputBuffers();
  ++global_seq;
  system_status.rx_packet_queue=packet_handler.pending_packets_size;
  DeferredPacketHandler_processPendingPackets(&packet_handler);
  if (system_params.periodic_packet_mask & PSystemStatusFlag) 
    Orazio_sendPacket((PacketHeader*)&system_status);
  if (system_params.periodic_packet_mask & PJointStatusFlag) {
    for (uint8_t i=0; i<NUM_JOINTS; ++i)
      Orazio_sendPacket((PacketHeader*)&joint_status[i]);
  }
  if (system_params.periodic_packet_mask & PDriveStatusFlag)
    Orazio_sendPacket((PacketHeader*)&drive_status);
  
#ifdef _ORAZIO_USE_SONAR_
  if ((system_params.periodic_packet_mask & PSonarStatusFlag) && Orazio_sonarReady()) 
    Orazio_sendPacket((PacketHeader*)&sonar_status);
#endif

  // if we have a string message to send to the host, we send it
  if(string_message.header.seq>0 && string_message.message[0]){
    Orazio_sendPacket((PacketHeader*)&string_message);
    string_message.header.seq=0;
  }
  Orazio_sendPacket((PacketHeader*)&end_epoch);
  Orazio_flushOutputBuffers();
}

void Orazio_commSendString(char* src){
  char* dest=string_message.message;
  int k=0;
  while(*src && k<MESSAGE_MAX_SIZE-1){
    *dest=*src;
    ++dest;
    ++src;
    ++k;
  }
  *dest=0;
  string_message.header.seq=1;
}
