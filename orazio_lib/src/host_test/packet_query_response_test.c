#include "packet_handler.h"
#include "serial_linux.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#pragma pack(push, 1)
typedef struct Packet0{
  PacketHeader header;
  float f;
} Packet0;
#pragma pack(pop)

#define PACKET0_TYPE 0
#define PACKET0_SIZE (sizeof(Packet0))


#pragma pack(push, 1)
typedef struct Packet1{
  PacketHeader header;
  float f1;
  float f2[3];
} Packet1;
#pragma pack(pop)

#define PACKET1_TYPE 1
#define PACKET1_SIZE (sizeof(Packet1))

Packet0 p0 = { {PACKET0_TYPE, PACKET0_SIZE, 0}, 0.0f };
Packet1 p1 = { {PACKET1_TYPE, PACKET1_SIZE, 0}, 0.1, {.1, .2, .3} };

PacketHandler packet_handler;


Packet0 packet0_buffer;
PacketHeader* Packet0_initializeBuffer(PacketType type,
				       PacketSize size,
				       void* args __attribute__((unused))) {
  if (type!=PACKET0_TYPE || size!=PACKET0_SIZE)
    return 0;
  return (PacketHeader*) &packet0_buffer;
}

int packet0_count=0;
PacketStatus Packet0_onReceive(PacketHeader* header,
			       void* args __attribute__((unused))) {
  ++header->seq;
  ++packet0_count;
  printf("0");
  fflush(stdout);
  if (packet0_count>20) {
    p1.header.seq=header->seq;
    PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &p1);
    packet0_count=0;
    printf("sync!\n");
  }
  return Success;
}

PacketOperations packet0_ops = {
  0,
  sizeof(Packet0),
  Packet0_initializeBuffer,
  0,
  Packet0_onReceive,
  0
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
  ++header->seq;
  printf("1");
  fflush(stdout);
  return Success;
}

PacketOperations packet1_ops = {
  1,
  sizeof(Packet1),
  Packet1_initializeBuffer,
  0,
  Packet1_onReceive,
  0
};

struct UART* uart;
int main(int argc, char** argv){
  assert(argc>1);
  int fd=serial_open(argv[1]);
  if(fd<0)
    return 0;
  if (serial_set_interface_attribs(fd, 115200, 0) <0)
    return 0;
  serial_set_blocking(fd, 1); 
  if  (! fd)
    return 0;
  
  PacketHandler_initialize(&packet_handler);
  PacketHandler_installPacket(&packet_handler, &packet0_ops);
  PacketHandler_installPacket(&packet_handler, &packet1_ops);
  for (int i=0; i<1000; ++i){
    volatile int packet_complete=0;
    while (! packet_complete) {
      uint8_t c;
      int n=read (fd, &c, 1);
      if (n) {
	PacketStatus status = PacketHandler_rxByte(&packet_handler, c);
	if (status<0)
	  printf("%d",status);
	fflush(stdout);
	packet_complete = (status==SyncChecksum);
      }
    }
    while(packet_handler.tx_size){
      uint8_t c=PacketHandler_txByte(&packet_handler);
      ssize_t res = write(fd,&c,1);
      usleep(1000);
    }
  }
}
