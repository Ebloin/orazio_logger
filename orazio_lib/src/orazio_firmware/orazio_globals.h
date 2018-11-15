#pragma once
#include "uart.h"
#include "deferred_packet_handler.h"
#include "orazio_packets.h"
#define ORAZIO_FIRMWARE_VERSION 0x20180805

//these global variables store the configuration
//the state and the control of each subsystem

extern  SystemParamPacket system_params;
extern  SystemStatusPacket system_status;

extern  DifferentialDriveParamPacket drive_params;
extern  DifferentialDriveStatusPacket drive_status;
extern  DifferentialDriveControlPacket drive_control;

extern JointParamPacket joint_params[NUM_JOINTS];
extern JointStatusPacket joint_status[NUM_JOINTS];
extern JointControlPacket joint_control[NUM_JOINTS];

extern  StringMessagePacket string_message;

#ifdef _ORAZIO_USE_SONAR_
extern  SonarStatusPacket sonar_status;
extern  SonarParamPacket  sonar_params;
#endif


