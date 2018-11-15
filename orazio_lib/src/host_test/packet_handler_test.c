#include "packet_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct Packet0{
  PacketHeader header;
  float f;
} Packet0;
#pragma pack(pop)

#define PACKET0_TYPE 0
#define PACKET0_SIZE (sizeof(Packet0))


PacketStatus Packet0_print(PacketHeader* header){
  Packet0* p =(Packet0 *) header;
  printf("Packet0{\n");
  printf("  seq: %d\n",(int) header->seq);
  printf("  f: %f\n",p->f);
  printf("}\n");
  return Success;
};

Packet0 packet0_buffer;
PacketHeader* Packet0_initializeBuffer(PacketType type,
                                       PacketSize size,
                                       void* args __attribute__((unused))) {
  if (type!=PACKET0_TYPE || size!=PACKET0_SIZE)
    return 0;
  return (PacketHeader*) &packet0_buffer;
}

PacketStatus Packet0_onReceive(PacketHeader* header,
                               void* args __attribute__((unused))) {
  printf("Packet0_onReceive\n");
  return Packet0_print(header);
}

PacketOperations packet0_ops = {
  0,
  sizeof(Packet0),
  Packet0_initializeBuffer,
  0,
  Packet0_onReceive,
  0
};

#pragma pack(push, 1)
typedef struct Packet1{
  PacketHeader header;
  float f1;
  float f2[3];
} Packet1;
#pragma pack(pop)

#define PACKET1_TYPE 1
#define PACKET1_SIZE (sizeof(Packet1))


PacketStatus Packet1_print(PacketHeader* header){
  Packet1* p =(Packet1 *) header;
  printf("Packet1{\n");
  printf("  seq: %d\n",(int) header->seq);
  printf("  f1: %f\n", p->f1);
  printf("  f2: [%f, %f, %f]\n", p->f2[0], p->f2[1], p->f2[2]);
  printf("}\n");
  return Success;
};

Packet1 packet1_buffer;
PacketHeader* Packet1_initializeBuffer(PacketType type,
                                       PacketSize size,
                                       void* args __attribute__((unused))) {
  if (type!=PACKET1_TYPE || size!=PACKET1_SIZE)
    return 0;
  return (PacketHeader*) &packet1_buffer;
}

PacketStatus Packet1_onReceive(PacketHeader* header,
                               void* args __attribute__((unused))) {
  printf("Packet1_onReceive\n");
  return Packet1_print(header);
}

PacketOperations packet1_ops = {
  1,
  sizeof(Packet1),
  Packet1_initializeBuffer,
  0,
  Packet1_onReceive,
  0
};

FILE* f=0;
PacketHandler packet_handler;
void flushBuffers(){
  while (packet_handler.tx_size)
    fputc(PacketHandler_txByte(&packet_handler), f);
}

int main(int argc, char** argv){

  int num_rounds=100;
  
  if (PacketHandler_initialize(&packet_handler) != Success) {
    printf("error initializing packet handler\n");
    exit (0);
  }

  if (PacketHandler_installPacket(&packet_handler, &packet0_ops) != Success) {
    printf("error installing handler\n");
    exit (0);
  }

  if (PacketHandler_installPacket(&packet_handler, &packet1_ops) != Success) {
    printf("error installing handler\n");
    exit (0);
  }

  if (argc<3) {
    printf("usage: %s [-w|-r] <filename>\n", argv[0]);
    exit(0);
  }
  int write=0;
  if (! strcmp(argv[1],"-w")){
    f=fopen(argv[2],"w");
    write=1;
  } else if (! strcmp(argv[1],"-r")){
    f=fopen(argv[2],"r");
  } else {
    printf("unknown_option, aborting\n");
    exit(0);
  }
  if (!f){
    printf("error opening file,  aborting\n");
    exit(0);
  }

  if (write) {
    int seq=0;
    Packet0 p0 = { {PACKET0_TYPE, PACKET0_SIZE, 0}, 0.0f };
    Packet1 p1 = { {PACKET1_TYPE, PACKET1_SIZE, 0}, 0.1, {.1, .2, .3} };
  
    for (int i=0; i<num_rounds; ++i){
      while (PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &p0) != Success) {
        printf("flushing (p0)\n");
        flushBuffers();
      }
      printf("p0");
      p0.header.seq=seq;
      seq++;
      while (PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &p1) != Success) {
        printf("flushing (p1)\n");
        flushBuffers();
      }
      printf("p1");
      p1.header.seq=seq;
      seq++;
    }
  } else {
    int c;
    while ( (c=fgetc(f))!=EOF) {
      uint8_t b=(uint8_t)c;
      int result=PacketHandler_rxByte(&packet_handler, b);
      if (result<0)
        printf("char: %02x, result: %d\n", (int) b, result);
    } 
  }
  
  fclose(f);
}
