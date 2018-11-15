#include "orazio_packets.h"
#include "orazio_joints_internal.h"

// this is a differential drive controller
// it hooks to two joints
// integrates encoder readings to compute the odometry
// based on the drive parameters
// converts the base velocity in joint ticks and smooths the velocity profile
typedef struct {
  // status of the differential drive
  DifferentialDriveStatusPacket* status;
  // parameters of differential drive
  const DifferentialDriveParamPacket* params;
  // control from differential drive
  const DifferentialDriveControlPacket* control;

  // status of left and right joints, the system reads from there the ticks on _driveUpdate
  const JointInfo* status_left, *status_right;

  // control of left and right joints, the system writes here the control, on _driveControl
  JointControl* control_left, *control_right;
  
  /* these quantities are cached from the parameters */
  // direct and inverse time interval in seconds
  float dt, idt;

  /*this is the joint controller dt*/
  float joints_dt;
  
  // ticks_to_meters for each wheen, read from params

  float kr, kl, ikr, ikl;

  // direct and inverse baseline, read from params
  float baseline, ibaseline;

  // maximum speed increments, translational and rotational
  //computed as dt*params->max_*_acceleration
  float max_dtv, max_drv;

  // encoder saved from the previous iteration
  int encoder_left_previous;
  int encoder_right_previous;
} DifferentialDriveController;

// initializes ctr by filling fields and computing cached quantities
void DifferentialDriveController_init(DifferentialDriveController* ctr,
                                      DifferentialDriveParamPacket* params,
                                      DifferentialDriveControlPacket* control,
                                      DifferentialDriveStatusPacket* status,
                                      JointController* controller_left,
                                      JointController* controller_right,
                                      float dt,
                                      float joints_dt
                                      );

// computes the control from the desired velocities in ctr->control,
// flushes them to the left and right joints ctr->control_left and ctr->control_right
void DifferentialDriveController_control(DifferentialDriveController* ctr);

// resets the platform controller to x,y,theta, and the velocities to 0
void DifferentialDriveController_reset(DifferentialDriveController* ctr,
                                       float x, float y, float theta);

// computes the odometry and the velocities reading from ctr->status_(left/right) and writing
// in ctr->status
void DifferentialDriveController_update(DifferentialDriveController* ctr);
