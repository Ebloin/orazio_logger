#pragma once

//initializes the communication
// opens the uart
// initializes the packet handler
// registers all packets and the corresponding handlers
void Orazio_commInit(void);

//flushes the input buffers
//calls the deferred actions
//initiates the transmission in the output buffers
void Orazio_commHandle(void);

// sends a packet (commodity function)
// returns a failure if there is no space in the buffers
PacketStatus Orazio_sendPacket(PacketHeader* p);

// sends a one shot string to the server through a StringMessagePacket;
void Orazio_commSendString(char* src);
