#pragma once
#include <stdint.h>
#include "orazio_packets.h"
#include "packet_operations.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct HBridgeOps;

  typedef struct HBridge{
    struct HBridgeOps* ops;
    union {
      struct {
        uint8_t pwm_pin;
        uint8_t dir_pin;
        int8_t brake_pin; // -1: disable
      } pwmdir;
      struct {
        uint8_t pwm_forward_pin;
        uint8_t pwm_backward_pin;
      } dualpwm;
      struct {
        uint8_t pwm_pin;
        uint8_t enable_pin;
      } halfpwm;
    } params;
  } HBridge;


  PacketStatus HBridge_setSpeed(struct HBridge* bridge, int16_t speed);

  /**
     type=PWMDir   :  pins[0]=pwm, pins[1]=dir, pins[2]=brake (-1=disabled);
     type=DualPWM  :  pins[0]=pwm_fwd, pins[1]=pwm_bwd;
     type=half_pwm :  pins[0]=pwm, pins[1]=enable (-1=disabled);
     @returns 0 on success, <0 on failure
  */
  PacketStatus HBridge_init(struct HBridge* bridge, const HBridgeType type, int8_t* pins);
  

#ifdef __cplusplus
}
#endif
