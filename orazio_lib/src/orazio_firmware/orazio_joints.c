#include <string.h>
#include "orazio_globals.h"
#include "encoder.h"
#include "orazio_joints_internal.h"
#include "orazio_hbridge_internal.h"


static HBridge bridges[NUM_JOINTS]={
  {
    .ops=0
  },
  {
    .ops=0
  }
};

PacketStatus Orazio_jointsInit(void){
  for (int i=0; i<NUM_JOINTS; ++i){
    memset(joint_controllers+i, 0, sizeof(JointController));
    JointController* c=joint_controllers+i;
    JointParams* params=&joint_params[i].param;
    JointInfo* status=&joint_status[i].info;
    JointControl* control=&joint_control[i].control;
    JointController_init(c, params, status, control);

    HBridge* bridge=bridges+i;
    HBridgeType type=params->h_bridge_type;
    PacketStatus r=HBridge_init(bridge, type, params->h_bridge_pins);
    if (r<0)
      return r;
  }
  return Success;
}

void Orazio_jointsHandle(void){
  // sample encoder values
  Encoder_sample();
  // compute desired speed based on the encoder input
  // and the control strategy
  for (uint8_t i=0; i<NUM_JOINTS; ++i){
    JointController_handle(joint_controllers+i, i);
  }
  // apply the control to each joint
  for (uint8_t i=0; i<NUM_JOINTS; ++i){
    HBridge* bridge=bridges+i;
    JointController* controller=joint_controllers+i;
    HBridge_setSpeed(bridge,controller->output);
  }
}

void Orazio_jointsDisable(void){
  for (uint8_t i=0; i<NUM_JOINTS; ++i){
    joint_controllers[i].control->mode=JointDisabled;
    joint_controllers[i].output=0;
  }
}
