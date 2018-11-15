#include "orazio_globals.h"
#include "orazio_drive_internal.h"
#include "orazio_joints_internal.h"

static DifferentialDriveController drive_controller;
volatile static uint16_t last_drive_control_seq=0; // last param seq receiveds

PacketStatus Orazio_driveInit(void){
  uint8_t left_idx=drive_params.left_joint_index;
  uint8_t right_idx=drive_params.right_joint_index;
  JointController* left_controller=joint_controllers+left_idx;
  JointController* right_controller=joint_controllers+right_idx;
  DifferentialDriveController_init(&drive_controller,
                                   &drive_params,
                                   &drive_control,
                                   &drive_status,
                                   left_controller,
                                   right_controller,
                                   0.001*system_params.timer_period_ms*system_params.comm_cycles,
                                   0.001*system_params.timer_period_ms);
  last_drive_control_seq=drive_control.header.seq;
  return Success;
}

void Orazio_driveUpdate(void){
  DifferentialDriveController_update(&drive_controller);
}

void Orazio_driveControl(void){
  if(last_drive_control_seq!=drive_control.header.seq) {
    drive_status.enabled=1;
    last_drive_control_seq=drive_control.header.seq;
  }
  if (drive_status.enabled){
    DifferentialDriveController_control(&drive_controller);
  }
}
