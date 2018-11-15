#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <pthread.h>
#include <fcntl.h>
#include <linux/joystick.h>

#include "orazio_client.h"
#include "orazio_print_packet.h"
#include "orazio_client_test_getkey.h"

#define NUM_JOINTS_MAX 4

typedef struct {
  int fd;
  double max_tv;
  double max_rv;
  int tv_axis;
  int rv_axis;
  int boost_button;
  int halt_button;
  const char* joy_device;
} JoyArgs;


const char* banner[] = {
  "orazio_client_test",
  " minimal program to connect to a configured orazio",
  " use arrow keys or joystick to steer the robot",
  " keys:",
  " 's':    toggles system mode",
  " 'j':    toggles joint mode",
  " 'd':    toggles drive mode",
  " 'r':    toggles sonar mode (ranges)",
  " 'ESC'   quits the program",
  "",
  "usage: orazio_client_test [options]",
  "options: ",
  "-serial-device <device>, (default: /dev/ttyACM0)",
  "-joy-device    <device>, (default: /dev/input/js0)",
  0
};

static void printMessage(const char** msg){
  while (*msg){
    printf("%s\n",*msg);
    ++msg;
  }
}

typedef enum {
  System=0,
  Joints=1,
  Drive=2,
  Ranges=3,
  None=4,
  Start=-1,
  Stop=-2
} Mode;

// display mode;
Mode mode=Start;
static struct OrazioClient* client=0;

// variables filled by the client when we ask for stuff (OrazioClient_get)
// or filled by us and sent to the client when we want to
// issue a command
// they are global since the keyboard/joystick threads might want to alter them
static DifferentialDriveControlPacket drive_control={
  .header.type=DIFFERENTIAL_DRIVE_CONTROL_PACKET_ID,
  .header.size=sizeof(DifferentialDriveControlPacket),
  .header.seq=1,
  .translational_velocity=0,
  .rotational_velocity=0
};

// to stop the robot we just schedule the sending
// of a drive message with zero velocities
// and we 
void stopRobot(void){
  drive_control.header.seq=0;
  drive_control.translational_velocity=0;
  drive_control.rotational_velocity=0;
  OrazioClient_sendPacket(client, (PacketHeader*)&drive_control, 0);
}

/** this stuff is to read the joystick**/
void* joyThread(void* args_){
  JoyArgs* args=(JoyArgs*)args_;
  args->fd = open (args->joy_device, O_RDONLY|O_NONBLOCK);
  if (args->fd<0) {
    printf ("no joy found on [%s]\n", args->joy_device);
    return 0;
  }
  printf("joy opened\n");
  printf("\t tv_axis: %d\n", args->tv_axis);
  printf("\t rv_axis: %d\n", args->rv_axis);
  printf("\t boost_button: %d\n", args->boost_button);
  printf("\t halt_button: %d\n", args->halt_button);
  printf("\t max_tv=%f\n", args->max_tv);
  printf("\t max_rv=%f\n", args->max_rv);

  float tv = 0;
  float rv = 0;
  float tvscale = args->max_tv/32767.0;
  float rvscale = args->max_rv/32767.0;
  float gain = 1;

  struct js_event e;
  while (mode!=Stop) {
    if (read (args->fd, &e, sizeof(e)) > 0 ){
      fflush(stdout);
      int axis = e.number;
      int value = e.value;
      int type = e.type;
      if (axis == args->tv_axis && type == 2) {
        tv = -value * tvscale;
      }
      if (axis == args->rv_axis && type == 2) {
        rv = -value *rvscale;
      }
      if (axis == args->halt_button && type==1){
        tv = 0;
        rv = 0;
      } else if (axis == args->boost_button && type == 1) {
        if(value)
          gain = 2.0;
        else
          gain = 1.0;
      } 
      drive_control.rotational_velocity=rv*gain;
      drive_control.translational_velocity=tv*gain;
      drive_control.header.seq=1;
    }
    usleep(10000); // 10 ms
  }
  close(args->fd);
  return 0;
};

void* keyThread(void* arg){
  setConioTerminalMode();
  while(mode!=Stop) {
    KeyCode key_code=getKey();
    switch (key_code){
    case KeyEsc:
      mode=Stop;
      break;
    case KeyArrowUp:
      drive_control.translational_velocity+=0.1;
      drive_control.header.seq=1;
      break;
    case KeyArrowDown:
      drive_control.translational_velocity-=0.1;
      drive_control.header.seq=1;
      break;
    case KeyArrowRight:
      drive_control.rotational_velocity-=0.1;
      drive_control.header.seq=1;
      break;
    case KeyArrowLeft:
      drive_control.rotational_velocity+=0.1;
      drive_control.header.seq=1;
      break;
    case KeyS:
      mode=System;
      break;
    case KeyR:
      mode=Ranges;
      break;
    case KeyJ:
      mode=Joints;
      break;
    case KeyD:
      mode=Drive;
      break;
    case KeyX:
      mode=None;
      break;
    default:
      stopRobot();
    }
  }
  resetTerminalMode();
  return 0;
};


char* default_joy_device="/dev/input/js0";
char* default_serial_device="/dev/ttyACM0";
  
int main(int argc, char** argv) {
  Orazio_printPacketInit();
  // parse the command line arguments
  int c=1;
  char* joy_device=default_joy_device;
  char* serial_device=default_serial_device;
    
  while(c<argc){
    if (! strcmp(argv[c],"-h")){
        printMessage(banner);
        exit(0);
    } else if (! strcmp(argv[c],"-serial-device")){
      ++c;
      if (c<argc)
        joy_device=argv[c];
    } else if (! strcmp(argv[c],"-joy-device")){
      ++c;
      if (c<argc)
        joy_device=argv[c];
    }
    ++c;
  }
  printf("Starting %s with the following parameters\n", argv[0]);
  printf("-serial-device %s\n", serial_device);
  printf("-joy-device %s\n", joy_device);

  
  // these variables are used locally in this thread
  // to interact with orazio client
  // it will read/write from/to these variables

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

  
  DifferentialDriveStatusPacket drive_status = {
    .header.type=DIFFERENTIAL_DRIVE_STATUS_PACKET_ID,
    .header.size=sizeof(DifferentialDriveStatusPacket)
  };

  JointStatusPacket joint_status[NUM_JOINTS_MAX];
  
  // 1. create an orazio object and initialize it
  client=OrazioClient_init(serial_device, 115200);
  if (! client) {
    printf("cannot open client on device [%s]\nABORTING", serial_device);
    return -1;
  }

  
  // 2. synchronize the serial protocol
  printf("Syncing ");
  for (int i=0; i<50; ++i){
    OrazioClient_sync(client,1);
    printf(".");
    fflush(stdout);
  }
  printf(" Done\n");

  // 3. read the configuration
  if (OrazioClient_readConfiguration(client, 100)!=Success){
    return -1;
  }

 
  // 4. ask how many motors are on the platform
  //    and initialize the index of each joint
  //    the client will read the index from the destination
  //    packet to know which joint is queried.
  int num_joints=OrazioClient_numJoints(client);
  for (int i=0; i<num_joints; ++i) {
    joint_status[i].header.header.type=JOINT_STATUS_PACKET_ID;
    joint_status[i].header.index=i;
  }
  int retries=10;
  
  // 5. at the beginning we disable sending all packets, to minimize the burden on the serial line
  //    a: ask the clients for the system params (read during readConfiguration)
  OrazioClient_get(client, (PacketHeader*)&system_params);
  //    b: set the parameter you want to change
  system_params.periodic_packet_mask=0;
  //    c: send the packet immediately, and try "retries" times, 
  OrazioClient_sendPacket(client, (PacketHeader*)&system_params, retries);
  //    d: get the parameters refreshed
  OrazioClient_get(client, (PacketHeader*)&system_params);

  

  // 6. we start a keyboard thread and a joystick thread
  //    these threads will write on
  //    drive_control and joint_control
  pthread_t key_thread;
  pthread_create(&key_thread, 0, keyThread, 0);

  pthread_t joy_thread;
  JoyArgs joy_args = {
    .max_tv=1,
    .max_rv=1,
    .tv_axis=1,
    .rv_axis=3,
    .boost_button=4,
    .halt_button=5,
    .joy_device=joy_device
  };
  pthread_create(&joy_thread, 0, joyThread,  &joy_args);
  
  Mode previous_mode=mode;
  // 7 the one below is the main loop
  //   the structure is:
  //   a. send in output, all controls you need
  //   b. sync
  //   c. get the variables/status from OrazioClient,
  //      by Orazio_get(cl, dest, packet_idx);
  //   If you need an immediate response after scheduling a command to be sent (with sendPacket)
  //   you need to issue the sync
  while(mode!=Stop){
    // we use the seq field of the drive_control packet
    // to notify we have to send a new control
    // if different from 0, we post the control packet

    // a. send the control
    if(drive_control.header.seq) {
      int result = OrazioClient_sendPacket(client, (PacketHeader*)&drive_control, 0);
      if (result)
        printf("send error\n");
      drive_control.header.seq=0;
    }
    
    //here we control the packets sent by the host, depending on the current mode
    if (previous_mode!=mode){
      switch(mode){
      case System:
        system_params.periodic_packet_mask=PSystemStatusFlag;
        break;
      case Joints:
        system_params.periodic_packet_mask=PJointStatusFlag;
        break;  
      case Ranges:
        system_params.periodic_packet_mask=PSonarStatusFlag;
        break;
      case Drive:
        system_params.periodic_packet_mask=PDriveStatusFlag;
        break;
      case None:
        system_params.periodic_packet_mask=0;
        break;
      default:;
      }
      // we send the new parameters (n retries)
      OrazioClient_sendPacket(client, (PacketHeader*)&system_params, retries);
      // we read the parameters again
      OrazioClient_get(client, (PacketHeader*)&system_params);
      previous_mode=mode;
    }
    // b. sync;
    OrazioClient_sync(client,1);

    // c. read the output (to visualize the data)
    char output_buffer[1024];
    int pos=0;
    switch(mode){
    case System:
      OrazioClient_get(client, (PacketHeader*)&system_status);
      Orazio_printPacket(output_buffer,(PacketHeader*)&system_status);
      printf("\r\033[2K%s",output_buffer);
      break;
    case Joints:
      for (int i=0; i<num_joints; ++i) {
        OrazioClient_get(client, (PacketHeader*)&joint_status[i]);
        pos+=Orazio_printPacket(output_buffer+pos,(PacketHeader*)&joint_status[i]);
        printf("\r\033[2K%s",output_buffer);
      }
      break;
    case Ranges:
      OrazioClient_get(client, (PacketHeader*)&sonar_status);
      Orazio_printPacket(output_buffer,(PacketHeader*)&sonar_status);
      printf("\r\033[2K%s",output_buffer);
      break;
    case Drive:
      OrazioClient_get(client, (PacketHeader*)&drive_status);
      Orazio_printPacket(output_buffer,(PacketHeader*)&drive_status);
      printf("\r\033[2K%s",output_buffer);
      break;
    default:;
    }
  }
  printf("Terminating\n");
  if (joy_args.fd>0){
    close(joy_args.fd);
  }
  // we reach this when we terminate
  void* arg;
  pthread_join(key_thread, &arg);
  pthread_join(joy_thread, &arg);

  printf("Stopping Robot");
  stopRobot();
  for (int i=0; i<10;++i){
    printf(".");
    fflush(stdout);
    OrazioClient_sync(client,10);
  }
  printf("Done\n");
  OrazioClient_destroy(client);
}
