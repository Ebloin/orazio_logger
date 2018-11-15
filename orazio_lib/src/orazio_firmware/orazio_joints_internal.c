#include "encoder.h"
#include "orazio_joints_internal.h"
JointController joint_controllers[NUM_JOINTS];

// limit a value between a minimum and a maximum
static inline int16_t clamp(int16_t v, int16_t value){
  if (v>value)
    return value;
  if (v<-value)
    return -value;
  return v;
}
void JointController_init(JointController* c,
                          JointParams*  params,
                          JointInfo*  status,
                          JointControl* control) {
  c->params=params;
  c->status=status;
  c->control=control;
}

void JointController_handle(JointController* j, uint8_t joint_num) {
  JointInfo* status=j->status;
  const JointControl* control=j->control;
  const JointParams* params=j->params;

  uint16_t enc=Encoder_getValue(joint_num);
  status->encoder_speed=(int16_t) (enc - status->encoder_position);
  status->encoder_position=enc;
  status->mode=control->mode;
  status->pwm=j->output;
  switch(control->mode){
  case JointDisabled: // inactive
    j->output=0;
    j->error_integral=0;
    break;

  case JointPWM: // pwm
    j->output=control->speed;
    break;

  case JointPID: // pid
    // clamp the reference to the maximum value
    status->desired_speed=clamp(control->speed, params->max_speed);
    // ramp
    j->error=clamp(status->desired_speed - status->encoder_speed, params->slope);
    j->ramp_reference=status->encoder_speed + j->error;
 
    // pid
    j->error_integral+= j->error;
    j->error_integral=clamp(j->error_integral,params->max_i);
    int16_t d_error=j->error-j->previous_error;
    j->output=j->ramp_reference+(params->kp*j->error
				 +params->ki*j->error_integral
				 +params->kd*d_error)/32;
    
    j->output=clamp(j->output, params->max_pwm);
    if (j->output<params->min_pwm && j->output>-params->min_pwm)
      j->output=0;
    
    j->previous_error=j->error;
    break;
  default:
    return;
  }
}
