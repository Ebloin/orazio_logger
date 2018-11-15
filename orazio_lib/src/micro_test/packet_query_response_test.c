#include <string.h>
#include "packet_handler.h"
#include "uart.h"
#include "delay.h"

#pragma pack(push, 1)
typedef struct Packet0{
  PacketHeader header;
  float f;
} Packet0;
#pragma pack(pop)

#define PACKET0_TYPE 0
#define PACKET0_SIZE (sizeof(Packet0))

PacketHandler packet_handler;


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
  ++header->seq;
  PacketHandler_sendPacket(&packet_handler, header);
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

#pragma pack(push, 1)
typedef struct Packet1{
  PacketHeader header;
  float f1;
  float f2[3];
} Packet1;
#pragma pack(pop)

#define PACKET1_TYPE 1
#define PACKET1_SIZE (sizeof(Packet1))

Packet1 packet1_buffer;
PacketHeader* Packet1_initializeBuffer(PacketType type,
				       PacketSize size,
				       void* args __attribute__((unused))) {
  if (type!=PACKET1_TYPE)
    return 0;
  return (PacketHeader*) &packet1_buffer;
}

PacketStatus Packet1_onReceive(PacketHeader* header,
			       void* args __attribute__((unused))) {
  ++header->seq;
  PacketHandler_sendPacket(&packet_handler, (PacketHeader*) header);
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

void flushInputBuffers(void) {
  while (UART_rxBufferFull(uart)){
    uint8_t c=UART_getChar(uart);
    PacketHandler_rxByte(&packet_handler, c);
  }
}

int flushOutputBuffers(void){
  while (packet_handler.tx_size)
    UART_putChar(uart, PacketHandler_txByte(&packet_handler));
  return packet_handler.tx_size;
}

int main(int argc, char** argv){
  uart = UART_init(0,115200);
  
  PacketHandler_initialize(&packet_handler);
  PacketHandler_installPacket(&packet_handler, &packet0_ops);
  PacketHandler_installPacket(&packet_handler, &packet1_ops);


  int seq=0;
  Packet0 p0 = { {PACKET0_TYPE, PACKET0_SIZE, 0}, 0.0f };
  while(1){
    flushInputBuffers();
    p0.header.seq=seq;
    seq++;
    PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &p0);
    delayMs(10);
    flushOutputBuffers();
  }
}
