#include "deferred_packet_handler.h"
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


typedef struct Packet1{
  PacketHeader header;
  float f1;
  float f2[3];
} Packet1;

DeferredPacketHandler packet_handler;

PacketStatus deferredAction(PacketHeader* header, void* args __attribute__((unused))){
  printf("action: %d, seq:%d, header: %p\n", header->type, header->seq, header);
  return Success;
}

#define PACKET0_TYPE 0
#define PACKET1_TYPE 1
#define PACKET0_NUM_BUFFERS 4
#define PACKET1_NUM_BUFFERS 4

static Packet0 packet0_buffers[PACKET0_NUM_BUFFERS];
static Packet1 packet1_buffers[PACKET1_NUM_BUFFERS];

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

  DeferredPacketHandler_initialize(&packet_handler);

  DeferredPacketHandler_installPacket(&packet_handler,
                                      PACKET0_TYPE,
                                      sizeof(Packet0),
                                      packet0_buffers,
                                      PACKET0_NUM_BUFFERS,
                                      deferredAction,
                                      0);
  DeferredPacketHandler_installPacket(&packet_handler,
                                      PACKET1_TYPE,
                                      sizeof(Packet1),
                                      packet1_buffers,
                                      PACKET1_NUM_BUFFERS,
                                      deferredAction,
                                      0);
  for (int i=0; i<1000; ++i){
    volatile int packet_complete=0;
    while (! packet_complete) {
      uint8_t c;
      int n=read (fd, &c, 1);
      if (n) {
        PacketStatus status = PacketHandler_rxByte(&packet_handler.base_handler, c);
        if (status<0){
          printf("error: %d\n", status);
        }
        packet_complete = (status==SyncChecksum);
      }
    }

    DeferredPacketHandler_processPendingPackets(&packet_handler);
    while(packet_handler.base_handler.tx_size){
      uint8_t c=PacketHandler_txByte(&packet_handler.base_handler);
      write(fd,&c,1);
    }

    usleep(20000);
  }
}
