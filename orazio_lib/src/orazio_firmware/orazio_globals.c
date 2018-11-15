#include "orazio_globals.h"
#include "orazio_pins.h"

//these packets are global
//variables that contain the state of our system
//and of the parameters
//they are updated automatically by the communication routines
//the status packets are updated and sent by various subsystems

SystemParamPacket system_params = {
  {.type=SYSTEM_PARAM_PACKET_ID,
   .size=sizeof(SystemParamPacket),
   .seq=0
  },
  .protocol_version=ORAZIO_PROTOCOL_VERSION,
  .firmware_version=ORAZIO_FIRMWARE_VERSION,
  .timer_period_ms=10,
  .comm_speed=115200,
  .comm_cycles=2,
  .periodic_packet_mask=(PSystemStatusFlag|PJointStatusFlag|PDriveStatusFlag|PSonarStatusFlag),
  .watchdog_cycles=0,
  .num_joints=NUM_JOINTS
};


DifferentialDriveParamPacket drive_params={
  {.type=DIFFERENTIAL_DRIVE_PARAM_PACKET_ID,
   .size=sizeof(DifferentialDriveParamPacket),
   .seq=0
  },
  .ikr=-10000,
  .ikl=10000,
  .baseline=0.405,
  .right_joint_index=0,
  .left_joint_index=1,
  .max_translational_velocity=1.,
  .max_translational_acceleration=3.,
  .max_translational_brake=4.,
  .max_rotational_velocity=2.,
  .max_rotational_acceleration=15.
};

SystemStatusPacket system_status = {
  {.type=SYSTEM_STATUS_PACKET_ID,
   .size=sizeof(SystemStatusPacket),
   .seq=0
  },
  .rx_seq=0,
  .rx_packet_queue=0,
  .idle_cycles=0
};


DifferentialDriveStatusPacket drive_status = {
  {.type=DIFFERENTIAL_DRIVE_STATUS_PACKET_ID,
   .size=sizeof(DifferentialDriveStatusPacket),
   .seq=0
  },
  .odom_x=0.,
  .odom_y=0.,
  .odom_theta=0.,
  .translational_velocity_measured=0.,
  .translational_velocity_desired=0.,
  .translational_velocity_adjusted=0.,
  .rotational_velocity_measured=0.,
  .rotational_velocity_desired=0.,
  .rotational_velocity_adjusted=0.,
  .enabled=0
};


DifferentialDriveControlPacket drive_control = {
  {.type=DIFFERENTIAL_DRIVE_CONTROL_PACKET_ID,
   .size=sizeof(DifferentialDriveControlPacket),
   .seq=0
  },
  .translational_velocity=0.,
  .rotational_velocity=0.
};

StringMessagePacket string_message = {
  {.type=MESSAGE_PACKET_ID,
   .size=sizeof(StringMessagePacket),
   .seq=0
  }
};


JointParamPacket joint_params[NUM_JOINTS]= {
  {// Joint 0
    {
      {
        .type=JOINT_PARAM_PACKET_ID,
        .size=sizeof(JointParamPacket),
        .seq=0
        },
      .index=0
    },
    {
      .kp=255,
      .ki=32,
      .kd=0,
      .max_i=255,
      .min_pwm=0,
      .max_pwm=255,
      .max_speed=100,
      .slope=10,
      .h_bridge_type=1,
      .h_bridge_pins[0]=H_BRIDGE_DUAL_PWM_0_FWD_PIN,
      .h_bridge_pins[1]=H_BRIDGE_DUAL_PWM_0_BWD_PIN,
      .h_bridge_pins[2]=-1,
      }
  },
  {// Joint 1
    {
      {
        .type=JOINT_PARAM_PACKET_ID,
        .size=sizeof(JointParamPacket),
        .seq=0
        },
      .index=1
    },
    {
      .kp=255,
      .ki=32,
      .kd=0,
      .max_i=255,
      .min_pwm=0,
      .max_pwm=255,
      .max_speed=100,
      .slope=10,
      .h_bridge_type=1,
      .h_bridge_pins[0]=H_BRIDGE_DUAL_PWM_1_FWD_PIN,
      .h_bridge_pins[1]=H_BRIDGE_DUAL_PWM_1_BWD_PIN,
      .h_bridge_pins[2]=-1
    }
  }
};

JointStatusPacket joint_status[NUM_JOINTS]= {
  {// Joint 0
    {
      {
        .type=JOINT_STATUS_PACKET_ID,
        .size=sizeof(JointStatusPacket),
        .seq=0
        },
      .index=0
    },
    {
      .desired_speed=0,
      .pwm=0,
      .sensed_current=0,
      .mode=JointDisabled
    }
  },
  {// Joint 1
    {
      {
        .type=JOINT_STATUS_PACKET_ID,
        .size=sizeof(JointStatusPacket),
        .seq=0
        },
      .index=1
    },
    {
      .desired_speed=0,
      .pwm=0,
      .sensed_current=0,
      .mode=JointDisabled
    }
  }
};

JointControlPacket joint_control[NUM_JOINTS] = {
  {// Joint 0
    {
      {
        .type=JOINT_CONTROL_PACKET_ID,
        .size=sizeof(JointControlPacket),
        .seq=0
        },
      .index=0
    },
    {
      .speed=0,
      .mode=JointDisabled
    }
  } ,
  {// Joint 1
    {
      {
        .type=JOINT_CONTROL_PACKET_ID,
        .size=sizeof(JointControlPacket),
        .seq=0
        },
      .index=1
    },
    {
      .speed=0,
      .mode=JointDisabled
    }
  }
};

#ifdef _ORAZIO_USE_SONAR_

SonarStatusPacket sonar_status = {
  {.type=SONAR_STATUS_PACKET_ID,
   .size=sizeof(SonarStatusPacket),
   .seq=0
  }
};

SonarParamPacket  sonar_params = {
  {.type=SONAR_PARAM_PACKET_ID,
   .size=sizeof(SonarParamPacket),
   .seq=0
  },
  .pattern={10,20,30,40,10,20,30,40}
};



#endif


 
