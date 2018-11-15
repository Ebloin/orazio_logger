#include <math.h>
#include "orazio_drive_internal.h"

#ifdef _DSPIC_
const float M_PI=3.141592654;
#endif

static const float cos_coeffs[]={0., 0.5 ,  0.    ,   -1.0/24.0,   0.    , 1.0/720.0};
static const float sin_coeffs[]={1., 0.  , -1./6. ,      0.    ,   1./120, 0.   };

// computes the terms of sin(theta)/thera and (1-cos(theta))/theta
// using taylor expansion
static void _computeThetaTerms(float* sin_theta_over_theta,
                               float* one_minus_cos_theta_over_theta,
                               float theta) {
  // evaluates the taylor expansion of sin(x)/x and (1-cos(x))/x,
  // where the linearization point is x=0, and the functions are evaluated
  // in x=theta
  *sin_theta_over_theta=0;
  *one_minus_cos_theta_over_theta=0;
  float theta_acc=1;
  for (uint8_t i=0; i<6; i++) {
    if (i&0x1)
      *one_minus_cos_theta_over_theta+=theta_acc*cos_coeffs[i];
    else 
      *sin_theta_over_theta+=theta_acc*sin_coeffs[i];
    theta_acc*=theta;
  }
}


static inline float _clamp(const float value, const float threshold) {
  if (value>threshold)
    return threshold;
  if (value<-threshold)
    return -threshold;
  return value;
}

void DifferentialDriveController_init(DifferentialDriveController* ctr,
                                      DifferentialDriveParamPacket* params,
                                      DifferentialDriveControlPacket* control,
                                      DifferentialDriveStatusPacket* status,
                                      JointController* controller_left,
                                      JointController* controller_right,
                                      float dt,
                                      float joints_dt) {
  ctr->params=params;
  ctr->status=status;
  ctr->control=control;
  ctr->status_left=controller_left->status;
  ctr->control_left=controller_left->control;
  ctr->status_right=controller_right->status;
  ctr->control_right=controller_right->control;
  ctr->dt=dt;
  ctr->idt=1./dt;
  ctr->joints_dt=joints_dt;
  ctr->ikr=params->ikr;
  ctr->ikl=params->ikl;
  ctr->kr=1./params->ikr;
  ctr->kl=1./params->ikl;
  ctr->baseline=params->baseline;
  ctr->ibaseline=1./params->baseline;
  ctr->max_dtv=ctr->params->max_translational_acceleration*dt;
  ctr->max_drv=ctr->params->max_rotational_acceleration*dt;
  DifferentialDriveController_reset(ctr, 0.,0.,0.);
}


// kinematic update of the controller
// reads from the joints and updates x,y,theta and measured velocities in
// in the drive
void DifferentialDriveController_update(DifferentialDriveController* ctr){
  const JointInfo* jls=ctr->status_left;
  const JointInfo* jrs=ctr->status_right;
  uint16_t encoder_right=jrs->encoder_position;
  uint16_t encoder_left=jls->encoder_position;
  int16_t left_ticks=encoder_left-ctr->encoder_left_previous;
  int16_t right_ticks=encoder_right-ctr->encoder_right_previous;
  ctr->encoder_right_previous=encoder_right;
  ctr->encoder_left_previous=encoder_left;

  float tvm=0;
  float rvm=0;
  // if the encoders say we don't move
  // we don't move
  if (left_ticks!=0 || right_ticks!=0){
    // left and right motion of the wheels in meters
    float delta_l=ctr->kl*left_ticks;
    float delta_r=ctr->kr*right_ticks;

    // save the encoder status in the controller for next cycle
  
    float delta_plus=delta_r+delta_l;
    float delta_minus=delta_r-delta_l;
    float dth= delta_minus * ctr->ibaseline;
    float one_minus_cos_theta_over_theta, sin_theta_over_theta;
    _computeThetaTerms(&sin_theta_over_theta, &one_minus_cos_theta_over_theta, dth);
    float dx=.5*delta_plus*sin_theta_over_theta;
    float dy=.5*delta_plus*one_minus_cos_theta_over_theta;

    //apply the increment to the previous estimate
    float s=sin(ctr->status->odom_theta);
    float c=cos(ctr->status->odom_theta);
    ctr->status->odom_x+=c*dx-s*dy;
    ctr->status->odom_y+=s*dx+c*dy;
    ctr->status->odom_theta+=dth;
    // normallize theta;
    if (ctr->status->odom_theta>M_PI)
      ctr->status->odom_theta-=2*M_PI;
    else if (ctr->status->odom_theta<-M_PI)
      ctr->status->odom_theta+=2*M_PI;

    tvm=.5*delta_plus*ctr->idt;
    rvm=dth*ctr->idt;
  }
  
  // velocity update
  ctr->status->translational_velocity_measured=tvm;
  ctr->status->rotational_velocity_measured=rvm;


  // control update
  float des_tv=ctr->control->translational_velocity;
  float des_rv=ctr->control->rotational_velocity;

  ctr->status->translational_velocity_desired=des_tv;
  ctr->status->rotational_velocity_desired=des_rv;
  float max_dtv=ctr->max_dtv;
  // if desired  velocities set to 0, enable the breaking acceleration threshold
  if(des_tv==0.0f){
    max_dtv=ctr->params->max_translational_brake;
  }

  // clamp desired speed according to acceleration profile
  float dtv=des_tv-tvm;
  dtv=_clamp(dtv,max_dtv);
  // adjust according to the accelerations
  des_tv=tvm+dtv;
  
  // clamp desired speed according to acceleration profile
  des_tv=_clamp(des_tv, ctr->params->max_translational_velocity);

  // clamp desired speed according to acceleration profile
  float drv=des_rv-rvm;
  drv=_clamp(drv,ctr->max_drv);
  // adjust according to the accelerations
  des_rv=rvm+drv;
  // clamp desired speed according to acceleration profile
  des_rv=_clamp(des_rv, ctr->params->max_rotational_velocity);

  ctr->status->translational_velocity_adjusted=des_tv;
  ctr->status->rotational_velocity_adjusted=des_rv;
}

void DifferentialDriveController_reset(DifferentialDriveController* ctr,
                                       float x, float y, float theta){
  ctr->status->odom_x=x;
  ctr->status->odom_y=y;
  ctr->status->odom_theta=theta;
  ctr->status->translational_velocity_measured=0.;
  ctr->status->rotational_velocity_measured=0.;
  ctr->status->translational_velocity_adjusted=0.;
  ctr->status->rotational_velocity_adjusted=0.;
  ctr->status->translational_velocity_desired=0.;
  ctr->status->rotational_velocity_desired=0.;
  ctr->encoder_left_previous=ctr->status_left->encoder_position;
  ctr->encoder_right_previous=ctr->status_right->encoder_position;
}

// Giorgio: this is my lil magic to get decent odometry
// integrate the motion along an arc the robot moves on
// avoid singularities at inf radius by using taylor expansion

void DifferentialDriveController_control(DifferentialDriveController* ctr){
  float dt=ctr->joints_dt;
  float tv=ctr->status->translational_velocity_adjusted;
  float rv=ctr->status->rotational_velocity_adjusted;
    
  // convert speed to distance sum/subtraction the 2 means the half of it
  float d2p  = tv*dt;
  float d2m = .5*ctr->params->baseline*rv*dt;
  float jlv  = ctr->ikl* (d2p - d2m);
  float jrv  = ctr->ikr* (d2p + d2m);
  
  // feedback to joint controls
  ctr->control_left->speed=jlv;
  ctr->control_right->speed=jrv;
  ctr->control_left->mode=JointPID;
  ctr->control_right->mode=JointPID;
}

