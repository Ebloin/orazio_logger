/*------------------------------------------------------------------------------
COMPILE WITH THE FOLLOWING:
gcc -Wall -Ofast --std=gnu99 -Iorazio_lib/src/common -Iorazio_lib/src/orazio_host/ -Iorazio_lib/src/host_test/ -c orazio_logger.c
gcc -Wall -Ofast --std=gnu99 -Iorazio_lib/src/common -Iorazio_lib/src/orazio_host/ -Iorazio_lib/src/host_test/ -o orazio_logger orazio_logger.o orazio_lib/host_build/orazio_client_test_getkey.o orazio_lib/host_build/packet_handler.o orazio_lib/host_build/deferred_packet_handler.o orazio_lib/host_build/orazio_client.o orazio_lib/host_build/orazio_print_packet.o orazio_lib/host_build/serial_linux.o -lpthread -lreadline -lwebsockets
------------------------------------------------------------------------------*/
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
#include <semaphore.h>
#include "orazio_client.h"
#include "orazio_print_packet.h"
#include "orazio_client_test_getkey.h"
#include "queue.h"
#include "webcam_manager.h"

#define NUM_JOINTS_MAX 4
#define MAX_BUFFER_SIZE 999999
#define MILLISECONDS_FRAME 50
#define GEN_DATA_ORAZIOPACKET 0
#define GEN_DATA_IMAGE 1


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

//Struttura pacchetto generico da aggiungere in coda
typedef struct genData{
  int type;
  int ts;	//timestamp
  void* data;
} genData_t;

//Modalità
typedef enum {
  System=0,
  Joints=1,
  Drive=2,
  Ranges=3,
  Start=-1,
  Stop=-2
} Mode;

//Strutture dei pacchetti da ricevere
static SystemStatusPacket system_status={
  .header.type=SYSTEM_STATUS_PACKET_ID,
  .header.size=sizeof(SystemStatusPacket)
};
static SystemParamPacket system_params={
  .header.type=SYSTEM_PARAM_PACKET_ID,
  .header.size=sizeof(SystemParamPacket)
};
static SonarStatusPacket sonar_status={
  .header.type=SONAR_STATUS_PACKET_ID,
  .header.size=sizeof(SonarStatusPacket)
};
static DifferentialDriveStatusPacket drive_status = {
  .header.type=DIFFERENTIAL_DRIVE_STATUS_PACKET_ID,
  .header.size=sizeof(DifferentialDriveStatusPacket)
};
static DifferentialDriveControlPacket drive_control={
  .header.type=DIFFERENTIAL_DRIVE_CONTROL_PACKET_ID,
  .header.size=sizeof(DifferentialDriveControlPacket),
  .header.seq=1,
  .translational_velocity=0,
  .rotational_velocity=0
};
static JointStatusPacket joint_status[NUM_JOINTS_MAX];

//VARIABILI
Mode mode=Start;
static struct OrazioClient* client=0;
char* default_joy_device="/dev/input/js0";
char* default_serial_device="/dev/ttyACM0";
sem_t empty, critical;

static void printMessage(const char** msg){
  while (*msg){
    printf("%s\n",*msg);
    ++msg;
  }
}

void stopRobot(void){
  drive_control.header.seq=0;
  drive_control.translational_velocity=0;
  drive_control.rotational_velocity=0;
  OrazioClient_sendPacket(client, (PacketHeader*)&drive_control, 0);
}

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
    default:
      stopRobot();
    }
  }
  resetTerminalMode();
  return 0;
};

void* printfileThread(void* args) {
  queue_t* q = (queue_t*)args;
  FILE* out;
  out = fopen("output.txt", "w+");
  if (out == NULL) {
    printf("Error on fopen\n");
    exit(1);
  }
  while (mode!=Stop) {
    //Controllo se la coda è vuota
    sem_wait(&empty);
    sem_wait(&critical);
    //printf("\n");
    genData_t* d = (genData_t*) dequeue(q);
    sem_post(&critical);
    int type = d->type;
    int timestamp = d->ts;
    switch (type) {
      case GEN_DATA_IMAGE:
        fprintf(out, "%d;\t\tFRAME;\t\t%s\n", timestamp, (char*)d->data);
        break;
      case GEN_DATA_ORAZIOPACKET:
        fprintf(out, "%d;\t\tPACKET;\t\t%s\n", timestamp, (char*)d->data);
        break;
    }
  }
  printf("Chiudo il file\n");
  fclose(out);
  return 0;
}

void * getpictureThread(void* args) {
  queue_t* q = (queue_t*) args;
  camera_t* camera = camera_open("/dev/video0", 352, 288);
  camera_init(camera);
  //do il tempo alla camera di inizializzarsi
  int i;
  for (i=0; i<10; i++) {
    camera_frame(camera);
  }
  mkdir("output_frames", ACCESSPERMS);

  while (!(mode == Stop && isempty(q))) {
    char nomefile [MAX_BUFFER_SIZE];
    camera_frame(camera);
    int timestamp = 0;
    //Getto il timestamp per sincronizzarlo con i pacchetti di marrtino
    switch (mode) {
      case System:
        timestamp = system_status.header.seq;
        break;
      case Joints:
      //------------------------------------> FIX!!!!!!
        timestamp = 0;
        break;
      case Ranges:
        timestamp = sonar_status.header.seq;
        break;
      case Drive:
        timestamp = drive_status.header.seq;
        break;
      default:;
    }
    sprintf(nomefile, "output_frames/%d.jpg", timestamp);
    unsigned char* rgb = yuyv2rgb(camera->frame.start, camera->width, camera->height);
    FILE* out = fopen(nomefile, "w");
    jpeg(out, rgb, camera->width, camera->height, 100);
    fclose(out);
    free(rgb);

    genData_t* new_packet = (genData_t*) malloc (sizeof(genData_t));
    new_packet->type = GEN_DATA_IMAGE;
    new_packet->ts = timestamp;
    new_packet->data = (void*) nomefile;
    sem_wait(&critical);
    enqueue(q, (void*)new_packet);
    sem_post(&empty);
    sem_post(&critical);
    usleep(MILLISECONDS_FRAME * 1000);
  }

  camera_stop(camera);
  return 0;
}


const char* banner[] = {
  "orazio_client_test",
  " minimal program to connect to a configured orazio",
  " use arrow keys or joystick to steer the robot",
  "usage: orazio-logger [options]",
  "options: ",
  "-serial-device <device>, (default: /dev/ttyACM0)",
  "-joy-device    <device>, (default: /dev/input/js0)",
  "-packet-type   <1, 2, 3, 4> (NOT OPTIONAL!)",
  0
};


//-------------MAIN FUNCTION---------------
int main(int argc, char** argv) {
  int ret;
  //Inizializzo i semafori per l'accesso alla coda
  ret = sem_init(&empty, 0, 0);
  if (ret == -1) {
    printf("Error opening empty sem\n");
    exit(1);
  }
  ret = sem_init(&critical, 0, 1);
  if (ret == -1) {
    printf("Error opening empty sem\n");
    exit(1);
  }

  //Inizializzo print_packet
  Orazio_printPacketInit();
  //Controllo sui dispositivi
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
    } else if (! strcmp(argv[c],"-packet-type")){
      ++c;
      //Scelta del tipo di pacchetto da loggare
      if (c<argc) {
        switch(atoi(argv[c])) {
          case 1:
            mode= System;
            printf("System_packets selected\n");
            break;
          case 2:
            mode= Joints;
            printf("Joints_packets selected\n");
            break;
          case 3:
            mode= Drive;
            printf("Drive_packets selected\n");
            break;
          case 4:
            mode= Ranges;
            printf("Ranges_packets selected\n");
            break;
        }
      }
    }
    ++c;
  }
  //Se non selezioni la modalità il programma termina
  if (mode == Start) {
    printf("Packet type parameter not inserted\nUse -packet-type parameter to select what kind of packets you want to log:\n1 -> System_packets\n2 -> Joints_packets\n3 -> Drive_packets\n4 -> Sonar_packets");
    exit(1);
  }
  printf("Starting %s with the following parameters\n", argv[0]);
  printf("-serial-device %s\n", serial_device);
  printf("-joy-device %s\n", joy_device);

  //Inizializzo la coda
  queue_t* data = createQueue();

  //Creo un orazio_client
  client=OrazioClient_init(serial_device, 115200);
  if (! client) {
    printf("cannot open client on device [%s]\nABORTING", serial_device);
    return -1;
  }

  printf("Syncing ");
  for (int i=0; i<50; ++i){
    OrazioClient_sync(client,1);
    printf(".");
    fflush(stdout);
  }
  printf(" Done\n");

  if (OrazioClient_readConfiguration(client, 100)!=Success){
    return -1;
  }

  //Inizializza l'indice di ogni giunto
  int num_joints=OrazioClient_numJoints(client);
  for (int i=0; i<num_joints; ++i) {
    joint_status[i].header.header.type=JOINT_STATUS_PACKET_ID;
    joint_status[i].header.index=i;
  }
  int retries=10;

  //Setto la periodic packet mask secondo i pacchetti che devo ricevere
  OrazioClient_get(client, (PacketHeader*)&system_params);
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
  default:;
  }
  OrazioClient_sendPacket(client, (PacketHeader*)&system_params, retries);
  OrazioClient_get(client, (PacketHeader*)&system_params);

  //Starto tutti i thread necessari
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

  pthread_t fileWriter_thread;
  pthread_create(&fileWriter_thread, 0, printfileThread, data);

  pthread_t camera_thread;
  pthread_create(&camera_thread, 0, getpictureThread, data);

  //----------------LOOP PRINCIPALE-------------------
  while(mode!=Stop){
    //Alloco la struttura pacchetto generio che sarà aggiunta nella coda
    genData_t* new_packet = (genData_t*) malloc (sizeof(genData_t));
    new_packet->type = GEN_DATA_ORAZIOPACKET;
	   //Invio il pacchetto drive_control se diverso dal precedente
    if(drive_control.header.seq) {
      int result = OrazioClient_sendPacket(client, (PacketHeader*)&drive_control, 0);
      if (result)
        printf("send error\n");
      drive_control.header.seq=0;
    }
    OrazioClient_sync(client,1);

    //Leggo l'output, lo visualizzo e lo aggiungo nella code di pacchetti
    char output_buffer[1024];
    int pos=0;
    switch(mode){
    case System:
      OrazioClient_get(client, (PacketHeader*)&system_status);
      Orazio_printPacket(output_buffer,(PacketHeader*)&system_status);
      new_packet->ts = system_status.header.seq;
      printf("\r\033[2K%s",output_buffer);
      break;
    case Joints:
      for (int i=0; i<num_joints; ++i) {
        OrazioClient_get(client, (PacketHeader*)&joint_status[i]);
        pos+=Orazio_printPacket(output_buffer+pos,(PacketHeader*)&joint_status[i]);
        //new_packet->ts = joint_status[i].header.seq;
        printf("\r\033[2K%s",output_buffer);
      }
      break;
    case Ranges:
      OrazioClient_get(client, (PacketHeader*)&sonar_status);
      Orazio_printPacket(output_buffer,(PacketHeader*)&sonar_status);
      new_packet->ts = sonar_status.header.seq;
      printf("\r\033[2K%s",output_buffer);
      break;
    case Drive:
      OrazioClient_get(client, (PacketHeader*)&drive_status);
      Orazio_printPacket(output_buffer,(PacketHeader*)&drive_status);
      new_packet->ts = drive_status.header.seq;
      printf("\r\033[2K%s",output_buffer);
      break;
    default:;
    }
    //AGGIUNGO IL PACCHETTO NELLA CODA
    new_packet->data = (void*) output_buffer;
    sem_wait(&critical);
    enqueue(data, (void*)new_packet);
    sem_post(&empty);
    sem_post(&critical);
  }

  //Entrato nella modalità STOP posso terminare
  //Chiudo file, telecamera e orazio
  printf("Terminating\n");
  if (joy_args.fd>0){
    close(joy_args.fd);
  }
  //Fine del lavoro
  void* arg;
  pthread_join(key_thread, &arg);
  pthread_join(joy_thread, &arg);
  pthread_join(fileWriter_thread, &arg);
  pthread_join(camera_thread, &arg);

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
