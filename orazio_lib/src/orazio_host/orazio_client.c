#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "orazio_client.h"
#include "orazio_print_packet.h"
#include "serial_linux.h"

#define NUM_JOINTS_MAX 4
const char* download_new_version_message[] ={
  "please download a fresh revision of client and firmware at",
  "  https://gitlab.com/srrg-software/srrg2_orazio_core",
  "compile and upload the firmware (on robot)",
  "and regenerate the host library (on PC)",
  0
};

const char* version_mismatch_message[] ={
  "ERROR: PROTOCOL VERSION MISMATCH, fatal!",
  0
};

const char* more_motors_message[] ={
  "ERROR: the platform declares more motors than I can handle!",
  "recompile THIS  source changing NUM_JOINTS_MAX in orazio_client.c",
  "to match the motors declared by the platform",
  0
};


static void printMessage(const char** msg) {
  while(*msg) {
    printf("%s\n",*msg);
    ++msg;
  }
};

typedef struct OrazioClient {
  PacketHandler packet_handler;
  uint16_t global_seq;
  
  //these are the system variables, updated by the serial communiction
  ResponsePacket response;
  StringMessagePacket message;
  EndEpochPacket end_epoch;
  SystemParamPacket system_param;
  SystemStatusPacket system_status;
  JointParamPacket joint_param[NUM_JOINTS_MAX];
  JointStatusPacket joint_status[NUM_JOINTS_MAX];
  DifferentialDriveParamPacket drive_param;
  DifferentialDriveStatusPacket drive_status;
  SonarStatusPacket sonar_status;
  SonarParamPacket sonar_param;
  
  // file descriptor of the serial port
  int fd;
  uint8_t packet_buffer[PACKET_SIZE_MAX];

  //sanity check on transmission
  PacketSize packet_sizes[PACKET_TYPE_MAX];
  pthread_mutex_t write_mutex;
  pthread_mutex_t read_mutex;

  // number of motors declared by the platform
  int num_joints;
  
  DifferentialDriveControlPacket drive_control_packet; // deferred drive control packet only one per comm cycle will be sent
  JointControlPacket joint_control[NUM_JOINTS_MAX];
  // deferred joint control packet only one per comm cycle will be sent

  int rx_bytes;
  int tx_bytes;
} 
  OrazioClient;

static PacketHeader* _initializeBuffer(PacketType type, PacketSize size, void* arg){
  OrazioClient* client=(OrazioClient*)arg;
  return (PacketHeader*) client->packet_buffer;
}
  
// this handler is called whenever a packet is complete
//! no deferred action will take place
static PacketStatus _copyToBuffer(PacketHeader* p, void* args) {
  memcpy(args, p, p->size);
  return Success;
}

static PacketStatus _copyToIndexedBuffer(PacketHeader* p, void* args) {
  PacketIndexed* p_idx=(PacketIndexed*)p;
  memcpy(args+p->size*p_idx->index, p, p->size);
  return Success;
}

static PacketStatus _installPacketOp(OrazioClient* cl,
                                     void* dest,
                                     PacketType type,
                                     PacketSize size,
                                     int indexed){
  
  PacketOperations* ops=(PacketOperations*)malloc(sizeof(PacketOperations));
  ops->type=type;
  ops->size=size;
  ops->initialize_buffer_fn=_initializeBuffer;
  ops->initialize_buffer_args=cl;
  if (indexed)
    ops->on_receive_fn=_copyToIndexedBuffer;
  else
    ops->on_receive_fn=_copyToBuffer;
  ops->on_receive_args=dest;
  PacketStatus install_result =
    install_result = PacketHandler_installPacket(&cl->packet_handler, ops);
  if (install_result!=Success) {
    printf("error in installing ops");
    exit(0);
    assert(0);
    free(ops);
  }
  return install_result;
}

static void _flushBuffer(OrazioClient* cl){
  while(cl->packet_handler.tx_size){
    uint8_t c=PacketHandler_txByte(&cl->packet_handler);
    ssize_t res = write(cl->fd,&c,1);
    cl->tx_bytes+=res;
  }
}

static void _readPacket(OrazioClient* cl){
  volatile int packet_complete=0;
  while (! packet_complete) {
    uint8_t c;
    int n=read (cl->fd, &c, 1);
    if (n) {
      fflush(stdout);
      PacketStatus status = PacketHandler_rxByte(&cl->packet_handler, c);
      if (0 && status<0){
        printf("error: %d\n", status);
      }
      packet_complete = (status==SyncChecksum);
    }
    cl->rx_bytes+=n;
  }
}

				    
OrazioClient* OrazioClient_init(const char* device, uint32_t baudrate){
  Orazio_printPacketInit();
  // tries to open and configure a device
  int fd=serial_open(device);
  if(fd<0)
    return 0;
  if (serial_set_interface_attribs(fd, baudrate, 0) <0)
    return 0;
  serial_set_blocking(fd, 1); 
  if  (! fd)
    return 0;
  
  OrazioClient* cl=(OrazioClient*) malloc(sizeof(OrazioClient));
  cl->global_seq=0;
  cl->fd=fd;
  cl->num_joints=0;
  cl->rx_bytes=0;
  cl->tx_bytes=0;
  
  // initializes the packets to send
  DifferentialDriveControlPacket ddcp={
    { .type=DIFFERENTIAL_DRIVE_CONTROL_PACKET_ID,
      .size=sizeof(DifferentialDriveControlPacket),
      .seq=0
    },
    .translational_velocity=0.f,
    .rotational_velocity=0.f
  };
  
  cl->drive_control_packet = ddcp;
  JointControlPacket jcp={
    {
      {
        .type=JOINT_CONTROL_PACKET_ID,
        .size=sizeof(JointControlPacket),
        .seq=0
      },
      .index=0
    },
    {.mode=JointDisabled,
     .speed=0
    }
  };
  
  for (int i=0; i<cl->num_joints; ++i) {
    cl->joint_param[i].header.index=i;
    cl->joint_status[i].header.index=i;
    jcp.header.index=i;
    cl->joint_control[i]=jcp;
  }
  
  
  
  // initializes the packet system
  PacketHandler_initialize(&cl->packet_handler);

  _installPacketOp(cl, &cl->response, RESPONSE_PACKET_ID, sizeof(cl->response), 0);
  _installPacketOp(cl, &cl->message, MESSAGE_PACKET_ID, sizeof(cl->message), 0);
  _installPacketOp(cl, &cl->system_param, SYSTEM_PARAM_PACKET_ID, sizeof(cl->system_param), 0);
  _installPacketOp(cl, &cl->system_status, SYSTEM_STATUS_PACKET_ID, sizeof(cl->system_status), 0);
  _installPacketOp(cl, cl->joint_status, JOINT_STATUS_PACKET_ID, sizeof(JointStatusPacket), 1);
  _installPacketOp(cl, cl->joint_param, JOINT_PARAM_PACKET_ID, sizeof(JointParamPacket), 1);
  _installPacketOp(cl, &cl->drive_param, DIFFERENTIAL_DRIVE_PARAM_PACKET_ID, sizeof(cl->drive_param), 0);
  _installPacketOp(cl, &cl->drive_status, DIFFERENTIAL_DRIVE_STATUS_PACKET_ID, sizeof(cl->drive_status), 0);
  _installPacketOp(cl, &cl->end_epoch, END_EPOCH_PACKET_ID, sizeof(cl->end_epoch), 0);
  _installPacketOp(cl, &cl->sonar_status, SONAR_STATUS_PACKET_ID, sizeof(cl->sonar_status), 0);
  _installPacketOp(cl, &cl->sonar_param, SONAR_PARAM_PACKET_ID, sizeof(cl->sonar_param), 0);
  // initialize the end epoch packet to make valgrind happy
  cl->end_epoch.type=END_EPOCH_PACKET_ID;
  cl->end_epoch.size=sizeof(cl->end_epoch);
  cl->end_epoch.seq=0;

  // initialize the response
  cl->response.header.type=RESPONSE_PACKET_ID;
  cl->response.header.size=sizeof(cl->response),
  cl->response.header.seq=0;
  cl->response.p_seq=0;
  cl->response.p_type=PACKET_TYPE_MAX;

  // initializes the outbound type/packet size
  memset(&cl->packet_sizes, 0, sizeof(cl->packet_sizes));
  cl->packet_sizes[PARAM_CONTROL_PACKET_ID]=sizeof(ParamControlPacket);
  cl->packet_sizes[SYSTEM_PARAM_PACKET_ID]=sizeof(SystemParamPacket);
  cl->packet_sizes[JOINT_PARAM_PACKET_ID]=sizeof(JointParamPacket);
  cl->packet_sizes[JOINT_CONTROL_PACKET_ID]=sizeof(JointControlPacket);
  cl->packet_sizes[DIFFERENTIAL_DRIVE_PARAM_PACKET_ID]=sizeof(DifferentialDriveParamPacket);
  cl->packet_sizes[DIFFERENTIAL_DRIVE_CONTROL_PACKET_ID]=sizeof(DifferentialDriveControlPacket);
  cl->packet_sizes[SONAR_PARAM_PACKET_ID]=sizeof(SonarParamPacket);
  pthread_mutex_init(&cl->write_mutex,NULL);
  pthread_mutex_init(&cl->read_mutex,NULL);
  return cl;
}


void OrazioClient_destroy(OrazioClient* cl){
  close(cl->fd);
  for (int i=0; i<PACKET_TYPE_MAX; ++i)
    if (cl->packet_handler.operations[i]){
      free(cl->packet_handler.operations[i]);
    }
  pthread_mutex_destroy(&cl->write_mutex);
  pthread_mutex_destroy(&cl->read_mutex);
  free(cl);
}

uint8_t OrazioClient_numJoints(struct OrazioClient* cl) {
  return cl->num_joints;
}

static PacketStatus _sendPacket(OrazioClient* cl, PacketHeader* p){
  ++cl->global_seq;
  PacketType type=p->type;
  if(type>=PACKET_TYPE_MAX)
    return UnknownType;
  PacketSize expected_size=cl->packet_sizes[type];
  if(! expected_size)
    return UnknownType;
  if(p->size!=expected_size)
    return InvalidSize;
  p->seq=cl->global_seq;
  return PacketHandler_sendPacket(&cl->packet_handler, p);
}

PacketStatus OrazioClient_sendPacket(OrazioClient* cl, PacketHeader* p, int timeout){
  PacketStatus send_result=GenericError;
  // non blocking operation
  if (! timeout) {
    pthread_mutex_lock(&cl->write_mutex);
    send_result=_sendPacket(cl,p);
    pthread_mutex_unlock(&cl->write_mutex);
    return send_result;
  }

  //blocking operation
  // no one else can write or read, we read packets
  // until timeout or until the response is received
  pthread_mutex_lock(&cl->write_mutex);
  pthread_mutex_lock(&cl->read_mutex);  
  send_result=_sendPacket(cl,p);
  if(send_result!=Success) {
    goto safe_exit;
  }
  _flushBuffer(cl);
  uint16_t awaited_seq=p->seq;
  uint16_t awaited_type=p->type;
  send_result = Timeout;
  while(timeout>0){
    _readPacket(cl);
    if (cl->response.p_type==awaited_type
        && cl->response.p_seq==awaited_seq){
      send_result = Success;
      break;
    }
    timeout--;
  }
 safe_exit:
  pthread_mutex_unlock(&cl->read_mutex);
  pthread_mutex_unlock(&cl->write_mutex);
  return send_result;
}

PacketStatus OrazioClient_sync(OrazioClient* cl, int cycles) {
  for (int c=0; c<cycles; ++c){
    pthread_mutex_lock(&cl->write_mutex);
    _flushBuffer(cl);
    if(cl->drive_control_packet.header.seq)
      _sendPacket(cl, (PacketHeader*) (&cl->drive_control_packet));
    pthread_mutex_unlock(&cl->write_mutex);
    uint16_t current_seq=cl->end_epoch.seq;
    do {
      pthread_mutex_lock(&cl->read_mutex);
      _readPacket(cl);
      pthread_mutex_unlock(&cl->read_mutex);
    } while (current_seq==cl->end_epoch.seq);
    //printf ("Sync! current_seq:%d\n", cl->end_epoch.seq);
  }
  return Success;
}

PacketStatus OrazioClient_readConfiguration(struct OrazioClient* cl, int timeout){

  ParamControlPacket query={
    {
      .type=PARAM_CONTROL_PACKET_ID,
      .size=sizeof(ParamControlPacket),
      .seq=0
    },
    .action=ParamRequest,
    .param_type=ParamSystem,
    .index=-1
  };
  PacketStatus status=OrazioClient_sendPacket(cl, (PacketHeader*)&query, timeout);
  printf("HOST:  Protocol version: %08x\n",  ORAZIO_PROTOCOL_VERSION);
  printf("HOST:  Max Motors:       %d\n",     NUM_JOINTS_MAX);

  printf("Querying params\n");
  printf("\t[System] Status: %d\n", status);
  if (status!=Success)
    return status;

  printf("\t\tROBOT: Protocol version: %08x\n", cl->system_param.protocol_version);
  printf("\t\tROBOT: Firmware version: %08x\n", cl->system_param.firmware_version);
  cl->num_joints=cl->system_param.num_joints;
  printf("\t\tROBOT: Num Motors      : %d\n",    cl->num_joints);
  
  if (cl->system_param.protocol_version!=ORAZIO_PROTOCOL_VERSION){
    printMessage(version_mismatch_message);
    printMessage(download_new_version_message);
    exit(-1);
  }

  if  (cl->num_joints>NUM_JOINTS_MAX) {
    printMessage(more_motors_message);
    exit(-1);
  };
  
  for (int i=0; i<cl->num_joints; ++i) {
    query.param_type=ParamJointsSingle;
    query.index=i;
    status=OrazioClient_sendPacket(cl, (PacketHeader*)&query, timeout);
    printf("\t[JointsSingle%d] Status: %d\n", i, status);
    if (status!=Success)
      return status;
  }
  query.index=-1;
  
  query.param_type=ParamDrive;
  status=OrazioClient_sendPacket(cl, (PacketHeader*)&query, timeout);
  printf("\t[Drive] Status: %d\n", status);
  if (status!=Success)
    return status;

  query.param_type=ParamSonar;
  status=OrazioClient_sendPacket(cl, (PacketHeader*)&query, timeout);
  printf("\t[Sonars] Status: %d\n", status);
  if (status!=Success)
    return status;

  
  printf("Done\n");
  return status;
}

//gets a packet from Orazio client
PacketStatus OrazioClient_get(struct OrazioClient* cl, PacketHeader* dest){
  PacketType type=dest->type;
  if (type>=PACKET_TYPE_MAX)
    return PacketTypeOutOfBounds;
  // all packets received have been registered in the handler ops
  // and the var on_receive_args of the buffer points to the correct variable
  const PacketOperations* ops=cl->packet_handler.operations[type];
  if (! ops){
    return UnknownType;
  }
  assert(ops->type==type);
  pthread_mutex_lock(&cl->read_mutex);
  if (ops->on_receive_fn==_copyToBuffer)
    memcpy(dest, ops->on_receive_args, ops->size);
  else {
    // we retrieve the index from the destination packet
    PacketIndexed* p_idx=(PacketIndexed*) dest;
    int index=p_idx->index;
    if (index<0||index>=cl->num_joints)
      return GenericError;
    memcpy(dest, ops->on_receive_args+index*ops->size, ops->size);
  }
  pthread_mutex_unlock(&cl->read_mutex);
  return Success;
}

void OrazioClient_setBaseVelocity(struct OrazioClient* cl, float tv, float rv){
  pthread_mutex_lock(&cl->write_mutex);
  cl->drive_control_packet.translational_velocity=tv;
  cl->drive_control_packet.rotational_velocity=rv;
  cl->drive_control_packet.header.seq=1;
  pthread_mutex_unlock(&cl->write_mutex);
}

void OrazioClient_getOdometryPosition(struct OrazioClient* cl, float* x, float* y, float* theta){
  pthread_mutex_lock(&cl->read_mutex);
  *x=cl->drive_status.odom_x;
  *y=cl->drive_status.odom_y;
  *theta=cl->drive_status.odom_theta;
  pthread_mutex_unlock(&cl->read_mutex);
}

