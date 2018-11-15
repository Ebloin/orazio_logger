#include "orazio_hbridge_internal.h"
#include "digio.h"
#include "pwm.h"
#include "orazio_packets.h"

typedef void (*HBridgeFiniFn)(HBridge* bridge);
typedef void (*HBridgeSetSpeedFn)(HBridge* bridge, int16_t speed);

typedef struct HBridgeOps{
  HBridgeFiniFn fini_fn;
  HBridgeSetSpeedFn setSpeed_fn;
}  HBridgeOps;



void HBridgePWMDir_fini(HBridge* bridge);
void HBridgePWMDir_setSpeed(HBridge* bridge, int16_t speed);

void HBridgeDualPWM_fini(HBridge* bridge);
void HBridgeDualPWM_setSpeed(HBridge* bridge, int16_t speed);

void HBridgeHalfPWM_fini(HBridge* bridge);
void HBridgeHalfPWM_setSpeed(HBridge* bridge, int16_t speed);


static HBridgeOps h_bridge_ops[]=
  {
    // PWM and dir
    {
      .fini_fn=HBridgePWMDir_fini,
      .setSpeed_fn=HBridgePWMDir_setSpeed
    },
    // dual pwm
    {
      .fini_fn=HBridgeDualPWM_fini,
      .setSpeed_fn=HBridgeDualPWM_setSpeed
    },
    // half pwm
    {
      .fini_fn=HBridgeHalfPWM_fini,
      .setSpeed_fn=HBridgeHalfPWM_setSpeed
    }
  };


/* PWM+Dir Mode */
PacketStatus HBridgePWMDir_init(HBridge* bridge, int8_t* pins){
  if (bridge->ops && bridge->ops->fini_fn) 
    (*bridge->ops->fini_fn)(bridge);
  
  int8_t  *pwm_pin=&pins[0];
  int8_t  *dir_pin=&pins[1];
  int8_t  *brake_pin=&pins[2];
  
  bridge->ops=h_bridge_ops+HBridgeTypePWMDir;

  if (DigIO_setDirection(*dir_pin,Output)<0
      || DigIO_setValue(*dir_pin,0) < 0) {
    *dir_pin=bridge->params.pwmdir.dir_pin;
    return WrongPins;
  }
  bridge->params.pwmdir.dir_pin=*dir_pin;

  if (*brake_pin>=0) {
    if (DigIO_setDirection(*brake_pin,Output)<0
        || DigIO_setValue(*brake_pin,0) < 0){
      *brake_pin=bridge->params.pwmdir.brake_pin;
      return WrongPins;
    }
    DigIO_setValue(*brake_pin,0); 
  }
  bridge->params.pwmdir.brake_pin=*brake_pin;
  
  if (PWM_enable(*pwm_pin,1)<0
      || PWM_setDutyCycle(*pwm_pin, 0)<0) {
    *pwm_pin=bridge->params.pwmdir.pwm_pin;
    return WrongPins;
  }
  bridge->params.pwmdir.pwm_pin=*pwm_pin;
  return Success;
}

void HBridgePWMDir_fini(HBridge* bridge){
  PWM_setDutyCycle(bridge->params.pwmdir.pwm_pin,0);
  PWM_enable(bridge->params.pwmdir.pwm_pin,0);
  DigIO_setValue(bridge->params.pwmdir.dir_pin,0);
  DigIO_setDirection(bridge->params.pwmdir.dir_pin,Input);
  if (bridge->params.pwmdir.brake_pin>=0){
    DigIO_setValue(bridge->params.pwmdir.brake_pin, 0);
    DigIO_setDirection(bridge->params.pwmdir.brake_pin, Input);
  }
}

void HBridgePWMDir_setSpeed(HBridge* bridge, int16_t speed){
  const int8_t dir_pin = bridge->params.pwmdir.dir_pin;
  const int8_t pwm_pin = bridge->params.pwmdir.pwm_pin;
  uint16_t pwm=0;
  uint8_t dir=0;
  if(speed>=0){
    pwm=speed;
    dir=0;
  } else {
    pwm=-speed;
    dir=1;
  }
  if (pwm>255)
    pwm=255;
  DigIO_setValue(dir_pin, dir);
  PWM_setDutyCycle(pwm_pin, pwm);
}

/* Dual PWM */
PacketStatus HBridgeDualPWM_init(HBridge* bridge, int8_t* pins){
  if (bridge->ops)
    (*bridge->ops->fini_fn)(bridge);

  int8_t* pwm_forward_pin=&pins[0];
  int8_t* pwm_backward_pin=&pins[1];
  
  bridge->ops=h_bridge_ops+HBridgeTypeDualPWM;
  if (PWM_enable(*pwm_forward_pin,1)<0
      || PWM_setDutyCycle(*pwm_forward_pin,0)<0) {
    *pwm_forward_pin = bridge->params.dualpwm.pwm_forward_pin;
    return WrongPins;
  }
  bridge->params.dualpwm.pwm_forward_pin=*pwm_forward_pin;

  if (PWM_enable(*pwm_backward_pin,1)<0
      || PWM_setDutyCycle(*pwm_backward_pin,0)<0) {
    *pwm_backward_pin = bridge->params.dualpwm.pwm_backward_pin;
  }
  bridge->params.dualpwm.pwm_backward_pin = *pwm_backward_pin;
  return Success;
}

void HBridgeDualPWM_fini(HBridge* bridge){
  PWM_setDutyCycle(bridge->params.dualpwm.pwm_forward_pin,0);
  PWM_enable(bridge->params.dualpwm.pwm_forward_pin,0);
  PWM_setDutyCycle(bridge->params.dualpwm.pwm_backward_pin,0);
  PWM_enable(bridge->params.dualpwm.pwm_backward_pin,0);
}

void HBridgeDualPWM_setSpeed(HBridge* bridge, int16_t speed){
  const uint8_t fpwm_pin = bridge->params.dualpwm.pwm_forward_pin;
  const uint8_t bpwm_pin = bridge->params.dualpwm.pwm_backward_pin;
  if (speed>255)
    speed=255;
  if (speed<-255)
    speed=-255;
  if(speed>=0){
    PWM_setDutyCycle(fpwm_pin, speed);
    PWM_setDutyCycle(bpwm_pin, 0);
  } else {
    PWM_setDutyCycle(fpwm_pin, 0);
    PWM_setDutyCycle(bpwm_pin, -speed);
  }
}

/* Half PWM */
PacketStatus HBridgeHalfPWM_init(HBridge* bridge, int8_t* pins){
  if (bridge->ops)
    (*bridge->ops->fini_fn)(bridge);

  int8_t *pwm_pin=&pins[0];
  int8_t *enable_pin=&pins[1];
  
  bridge->ops=h_bridge_ops+HBridgeTypeHalfCyclePWM;

  if (PWM_setDutyCycle(*pwm_pin,0)<0
      || PWM_enable(*pwm_pin,1)<0) {
    *pwm_pin=bridge->params.halfpwm.pwm_pin;
    return WrongPins;
  }
  bridge->params.halfpwm.pwm_pin=*pwm_pin;

  if ( *enable_pin>=0) {
    if( DigIO_setDirection(*enable_pin,Output)<0
        || DigIO_setValue(*enable_pin, 1) < 0) {
      *enable_pin = bridge->params.halfpwm.enable_pin;
      return WrongPins;
    }
  }
  bridge->params.halfpwm.enable_pin = *enable_pin;
  return Success;
}

void HBridgeHalfPWM_fini(HBridge* bridge){
  uint8_t pwm_pin=bridge->params.halfpwm.pwm_pin;
  uint8_t enable_pin=bridge->params.halfpwm.enable_pin;
  PWM_setDutyCycle(pwm_pin,0);
  DigIO_setValue(enable_pin, 0);
  if (enable_pin>=0) {
    PWM_enable(pwm_pin,0);
    DigIO_setDirection(enable_pin,Input);
  }
}

void HBridgeHalfPWM_setSpeed(HBridge* bridge, int16_t speed){
  uint8_t pwm_pin=bridge->params.halfpwm.pwm_pin;
  if (speed>255)
    speed=255;
  if (speed<-255)
    speed=-255;
  speed=127+speed/2;
  PWM_setDutyCycle(pwm_pin, speed);
}

PacketStatus HBridge_init(struct HBridge* bridge, const HBridgeType type, int8_t* pins){
  switch(type){
  case HBridgeTypePWMDir:
    return HBridgePWMDir_init(bridge, pins);
  case HBridgeTypeDualPWM:
    return HBridgeDualPWM_init(bridge, pins);
  case HBridgeTypeHalfCyclePWM:
    return HBridgeHalfPWM_init(bridge, pins);
  default:
    return -1;
  }
}

PacketStatus HBridge_setSpeed(HBridge* bridge, int16_t speed) {
  if (! bridge->ops)
    return GenericError;
  if (! bridge->ops->setSpeed_fn)
    return GenericError;
  (*bridge->ops->setSpeed_fn)(bridge, speed);
  return 0;
}
