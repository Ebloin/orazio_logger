#pragma once
#include "packet_handler.h"
#include "orazio_packets.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct OrazioClient;

  // creates a new orazio client, opening a serial connection on device at the selected baudrate
  struct OrazioClient* OrazioClient_init(const char* device, uint32_t baudrate);

  // destroyes a previously created orazio client
  void OrazioClient_destroy(struct OrazioClient* cl);

  // all functions below support multithreading
  
  // sends a packet
  // if timeout ==0, the packet sending is deferred
  // otherwise it enables a synchronous operation that waits for timeout packets to be received
  PacketStatus OrazioClient_sendPacket(struct OrazioClient* cl, PacketHeader* p, int timeout);

  //gets a buffered packet from Orazio client
  // if the packet is NOT indexrd
  //    dest->header.type should be a vaild packet id
  // if the packet IS indexed
  //    dest->header.header.type and dest.header.index
  //    should contain the packet id and the index of the packet being read
  PacketStatus OrazioClient_get(struct OrazioClient* cl, PacketHeader* dest);

  // number of motors in the platform
  uint8_t OrazioClient_numJoints(struct OrazioClient* cl);
  
  // flushes the deferred tx queues,
  // reads all packets of an epoch (same seq),
  // and returns
  // call it periodically
  PacketStatus OrazioClient_sync(struct OrazioClient* cl, int cycles);

  //to be called at the beginning after a few loops of sync
  PacketStatus OrazioClient_readConfiguration(struct OrazioClient* cl, int timeout);

  // sugar for differentual drive control
  PacketStatus OrazioClient_setBaseVelocities(struct OrazioClient* cl, float tv, float rv);

  PacketStatus OrazioClient_getBasePosition(struct OrazioClient* cl, float*x, float*y, float*theta);

#ifdef __cplusplus
}
#endif
