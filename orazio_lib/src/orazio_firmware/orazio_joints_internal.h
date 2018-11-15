#pragma once
#include "orazio_globals.h"

// controller for a joint
typedef struct {
  JointParams*  params;
  JointInfo*  status;
  JointControl* control; 
  int16_t output;
  int16_t ramp_reference;
  int16_t reference;
  int16_t error_integral;
  int16_t error;
  int16_t previous_error;
} JointController;

// updates the status of a joint and computes the new output
// based on the control value
void JointController_handle(JointController* j, uint8_t joint_num);

// global list of controllers
extern JointController joint_controllers[NUM_JOINTS];

void JointController_init(JointController* controller,
                          JointParams*  params,
                          JointInfo*  status,
                          JointControl* control);
