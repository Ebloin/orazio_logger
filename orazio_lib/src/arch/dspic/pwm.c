#include <p33FJ128MC802.h>
#include "pwm.h"
#include "pins.h"

#define NUM_CHANNELS 14

//ia polarity check on reset
_FPOR(LPOL_ON & HPOL_ON);


PWMError PWM_init(void){  
  //ia setup timers
  //ia here we use timers 0-4, in 8 bit mode
  //ia regardless if they are 16 bit
  //ia for updown count mode: P1TPER = FCY/(Fpwm*P1TMRprescaler*2)-1
  //ia FCY = 16000000; Fpwm = 20000; P1TMR = 1 => P1TPER = 499
  uint16_t period = 499;
  
  // disable all user interrupts (atomically)
  asm volatile ("disi #0x3FFF"); 
  P1TPERbits.PTPER = period;  // PWM Time Base Period Value
  P1SECMPbits.SEVTCMP = 0;    // Special Event Compare Value - default
  
  P1TCONbits.PTEN = 1;        // PWM1 Time Base Timer Enable bit
  P1TCONbits.PTSIDL = 0;      // PWM time base runs in CPU idle mode
  P1TCONbits.PTOPS = 0;       // PWM time base output postscale 1:1
  P1TCONbits.PTCKPS = 0;      // PWM time base input clock period is TCY (1:1 prescale)
  P1TCONbits.PTMOD = 0b00;    // PWM time base operates in Free Mode [00] -> 
                              //     if you change this, then period must be adjusted 

  PWM1CON1bits.PMOD1 = 1;     // PWM1 in indipendent mode
  PWM1CON1bits.PMOD2 = 1;     // PWM2 in indipendent mode
//  PWM1CON1bits.PMOD3 = 1;     // PWM3 in indipendent mode
  
  PWM1CON1bits.PEN1H = 1;
  PWM1CON1bits.PEN1L = 0;
  PWM1CON1bits.PEN2H = 1;
  PWM1CON1bits.PEN2L = 0;
  PWM1CON1bits.PEN3H = 0;
  PWM1CON1bits.PEN3L = 0;
  
  P1OVDCONbits.POVD3H = 0;
  P1OVDCONbits.POVD3L = 0;
  P1OVDCONbits.POVD2H = 1;
  P1OVDCONbits.POVD2L = 0;
  P1OVDCONbits.POVD1H = 1;
  P1OVDCONbits.POVD1L = 0;
  
  PWM1CON2bits.IUE = 1;       // immediate update of PWM enabled
  
  P1TMR = 0;                  // start counting from 0
  asm volatile ("disi #0"); 
  
  return PWMSuccess;
}

// how many pwm on this chip?
uint8_t PWM_numChannels(void){
  return PINS_NUM;
}

// what was the period i set in the pwm subsystem
// might only require to adjust the prescaler
PWMError PWM_isEnabled(uint8_t c) {
  if (c>=PINS_NUM)
    return PWMChannelOutOfBound;
  const Pin* pin = pins+c;
  if (!pin->tcc_register)
    return PWMChannelOutOfBound;
  if ((*pin->tcc_register & pin->com_mask)==0)
    return 0;
  return PWMEnabled;
}

// sets the output on a pwm channel
PWMError PWM_enable(uint8_t c, uint8_t enable){
  if (c>=PINS_NUM)
    return PWMChannelOutOfBound;
  const Pin* pin = pins+c;
  if (!pin->tcc_register)
    return PWMChannelOutOfBound;
  *pin->oc_register=0;
  if (enable){
    *pin->tcc_register |= pin->com_mask;
    *pin->dir_register |= (1<<pin->bit);
  } else {
    *pin->tcc_register &= ~pin->com_mask;
    *pin->dir_register &= ~(1<<pin->bit);
  }
  return PWMSuccess;
}


// what was the duty cycle I last set?
int16_t PWM_getDutyCycle(uint8_t c){
  if (c>=PINS_NUM)
    return PWMChannelOutOfBound;
  const Pin* pin = pins+c;
  if (!pin->tcc_register)
    return PWMChannelOutOfBound;
  return (*pin->oc_register)>>3;
}

// sets the duty cycle
 PWMError PWM_setDutyCycle(uint8_t c, uint8_t duty_cycle){
  if (c>=PINS_NUM)
    return PWMChannelOutOfBound;
  const Pin* pin = pins+c;
  if (!pin->tcc_register)
    return PWMChannelOutOfBound;
  *pin->oc_register = duty_cycle<<3;
  return PWMSuccess;
}
