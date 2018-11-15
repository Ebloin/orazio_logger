#include <p33FJ128MC802.h>
#include "digio.h"
#include "pins.h"
#define NUM_CHANNELS 14  

// initializes the digital io pins of the chip
void DigIO_init(void) {
//ia this two register override the analog/digital thing on the pins
//ia be careful with those, for now they are not required to be set
//  AD1PCFGL = 0xFFFF;
//  P1OVDCON = 0x3F00;
  
  uint8_t num_channels=DigIO_numChannels();
  for (int i=0; i<num_channels; ++i) {
    // here we set all pis as input
    DigIO_setDirection(i,1);
    // pull up on each pin
    DigIO_setValue(i,1);
  }
}

// returns the number of digital io pins on the chip
uint8_t  DigIO_numChannels(void){
  return PINS_NUM; // fixed maximum number of pins mapped to arduino
}

int8_t DigIO_setDirection(uint8_t pin, PinDirection dir) {
  if (pin>=PINS_NUM)
    return -1;
  const Pin* mapping=pins+pin;
  if (!mapping->in_register)
    return -1;
  uint16_t mask=1<<mapping->bit;
  if (dir)
    *(mapping->dir_register)&=~mask;    // set pin as output (OPPOSITE of atmega)
  else
    *(mapping->dir_register)|=mask;     // set pin as input (OPPOSITE of atmega) 
  return 0;
}

int8_t DigIO_getDirection(uint8_t pin){
  if (pin>=PINS_NUM)
    return -1;
  const Pin* mapping=pins+pin;
  if (!mapping->in_register)
    return -1;
  
  uint16_t value=*(mapping->dir_register);
  return ((value >> pins[pin].bit)&0x1)?Input:Output;
}

int8_t DigIO_setValue(uint8_t pin, uint8_t value) {
  if (pin>=PINS_NUM)
    return -1;
  const Pin* mapping=pins+pin;
  if (!mapping->in_register)
    return -1;
  uint16_t mask=1<<mapping->bit;
  if (value)
    *(mapping->out_register)|=mask;
  else
    *(mapping->out_register)&=~mask;
  return 0;
}

int8_t DigIO_getValue(uint8_t pin){
  if (pin>=PINS_NUM)
    return -1;
  const Pin* mapping=pins+pin;
  if (!mapping->in_register)
    return -1;
  uint16_t value=*(mapping->in_register);
  return (value >> pins[pin].bit)&0x1;
}
