#pragma once
#include <stdio.h>
#include "orazio_packets.h"

#ifdef __cplusplus
extern "C" {
#endif

  void Orazio_printPacketInit();
  int Orazio_printPacket(char* buffer, PacketHeader* header);

#ifdef __cplusplus
}
#endif
