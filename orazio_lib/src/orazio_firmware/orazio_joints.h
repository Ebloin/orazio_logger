#pragma once
#include "orazio_globals.h"

// initializes the joint subsystems based on the configuration
// also initializes the h bridges and the pins
// returns <0 on error
PacketStatus Orazio_jointsInit(void);

// handles one update of the joints
void Orazio_jointsHandle(void);

// sets to 0 all joint controls
void Orazio_jointsDisable(void);
