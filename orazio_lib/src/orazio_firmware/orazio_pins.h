#pragma once
//! definition of the pins used in the controller board

#ifdef _ARDUINO_MEGA_2560_
// encoders on mega connected to pins
// ENC_0, A: 50, B: 51
// ENC_1, A: 52, B: 53

// arduino motor control board (mode 0)
#define H_BRIDGE_PWM_DIR_0_PWM_PIN    3
#define H_BRIDGE_PWM_DIR_0_DIR_PIN    12
#define H_BRIDGE_PWM_DIR_0_BRAKE_PIN  9
#define H_BRIDGE_PWM_DIR_1_PWM_PIN    11
#define H_BRIDGE_PWM_DIR_1_DIR_PIN    13
#define H_BRIDGE_PWM_DIR_1_BRAKE_PIN  8

//dual_pwm (mode 1)
#define H_BRIDGE_DUAL_PWM_0_FWD_PIN 2
#define H_BRIDGE_DUAL_PWM_0_BWD_PIN 3
#define H_BRIDGE_DUAL_PWM_1_FWD_PIN 4
#define H_BRIDGE_DUAL_PWM_1_BWD_PIN 5

// half PWM (mode 2)
#define H_BRIDGE_HALF_PWM_0_PWM_PIN 2
#define H_BRIDGE_HALF_PWM_0_EN_PIN  3
#define H_BRIDGE_HALF_PWM_1_PWM_PIN 4
#define H_BRIDGE_HALF_PWM_1_EN_PIN  5
#endif



#ifdef _DS_NAV_
//ia @brief the only mode that actually works is the pwm+dir
//ia        this corresponds to HBRIDGE-MODE 0 in the orazio html client
//ia CHECK ALWAYS PIN VALUES!!!!!
//ia  encoders on dsnav connected to encoder connector (A+B-)

//pwm+dir
#define H_BRIDGE_PWM_DIR_0_PWM_PIN    3
#define H_BRIDGE_PWM_DIR_0_DIR_PIN    2
#define H_BRIDGE_PWM_DIR_0_BRAKE_PIN  9
#define H_BRIDGE_PWM_DIR_1_PWM_PIN    5
#define H_BRIDGE_PWM_DIR_1_DIR_PIN    4
#define H_BRIDGE_PWM_DIR_1_BRAKE_PIN  8

//dual_pwm -> not implemented on dspic
#define H_BRIDGE_DUAL_PWM_0_FWD_PIN 2
#define H_BRIDGE_DUAL_PWM_0_BWD_PIN 3
#define H_BRIDGE_DUAL_PWM_1_FWD_PIN 4
#define H_BRIDGE_DUAL_PWM_1_BWD_PIN 5

// half PWM -> not implemented on dspic
#define H_BRIDGE_HALF_PWM_0_PWM_PIN 2
#define H_BRIDGE_HALF_PWM_0_EN_PIN  3
#define H_BRIDGE_HALF_PWM_1_PWM_PIN 4
#define H_BRIDGE_HALF_PWM_1_EN_PIN  5

#endif

/** not implemented fully, only compiled firmware **/

#ifdef _ARDUINO_328_
//ia TODO FIX THE VALUES, they are just placeholders
// encoders on mega connected to pins
// ENC_0, A: 50, B: 51
// ENC_1, A: 52, B: 53

// arduino motor control board (mode 0)
#define H_BRIDGE_PWM_DIR_0_PWM_PIN    3
#define H_BRIDGE_PWM_DIR_0_DIR_PIN    12
#define H_BRIDGE_PWM_DIR_0_BRAKE_PIN  9
#define H_BRIDGE_PWM_DIR_1_PWM_PIN    11
#define H_BRIDGE_PWM_DIR_1_DIR_PIN    13
#define H_BRIDGE_PWM_DIR_1_BRAKE_PIN  8

//dual_pwm
#define H_BRIDGE_DUAL_PWM_0_FWD_PIN 2
#define H_BRIDGE_DUAL_PWM_0_BWD_PIN 3
#define H_BRIDGE_DUAL_PWM_1_FWD_PIN 5
#define H_BRIDGE_DUAL_PWM_1_BWD_PIN 5

// half PWM
#define H_BRIDGE_HALF_PWM_0_PWM_PIN 2
#define H_BRIDGE_HALF_PWM_0_EN_PIN  3
#define H_BRIDGE_HALF_PWM_1_PWM_PIN 4
#define H_BRIDGE_HALF_PWM_1_EN_PIN  5
#endif

#ifdef _TEENSY_36_
// TODO: FIX PIN VALUES!!!!!
// the teensy has hardware quadrature encoders

#define H_BRIDGE_PWM_0_PWM_PIN    3
#define H_BRIDGE_PWM_0_DIR_PIN    12
#define H_BRIDGE_PWM_0_BRAKE_PIN  9
#define H_BRIDGE_PWM_1_PWM_PIN    11
#define H_BRIDGE_PWM_1_DIR_PIN    13
#define H_BRIDGE_PWM_1_BRAKE_PIN  8

//dual_pwm
#define H_BRIDGE_DUAL_PWM_0_FWD_PIN 2
#define H_BRIDGE_DUAL_PWM_0_BWD_PIN 3
#define H_BRIDGE_DUAL_PWM_1_FWD_PIN 5
#define H_BRIDGE_DUAL_PWM_1_BWD_PIN 5

// half PWM
#define H_BRIDGE_HALF_PWM_0_PWM_PIN 2
#define H_BRIDGE_HALF_PWM_0_EN_PIN  3
#define H_BRIDGE_HALF_PWM_1_PWM_PIN 4
#define H_BRIDGE_HALF_PWM_1_EN_PIN  5

#endif


