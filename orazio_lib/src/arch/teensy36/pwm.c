#include "pwm.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// here we use timers 0-4, in 8 bit mode
// regardless if they are 16 bit
#define PINS_NUM 13

#define TCCRA_MASK (1<<WGM10)               // fast PWM, non inverted
#define TCCRB_MASK ((1<<WGM12)|(1<<CS10))   // fast PWM, non inverted, 16Mhz

// initializes the pwm subsystem
PWMError PWM_init(void){
  return PWMSuccess;
}

// how many pwm on this chip?
uint8_t PWM_numChannels(void){
  return PINS_NUM;
}

// what was the period i set in the pwm subsystem
// might only require to adjust the prescaler
PWMError PWM_isEnabled(uint8_t c) {
  return PWMEnabled;
}

// sets the output on a pwm channel
PWMError PWM_enable(uint8_t c, uint8_t enable){
  return PWMSuccess;
}


// what was the duty cycle I last set?
int16_t PWM_getDutyCycle(uint8_t c){
  return 255;
}

// sets the duty cycle
 PWMError PWM_setDutyCycle(uint8_t c, uint8_t duty_cycle){
  return PWMSuccess;
}

