#include "digio.h"
#include "pins.h"
#include <avr/io.h>
#define NUM_CHANNELS 14

// initializes the digital io pins of the chip
void DigIO_init(void) {
}

// returns the number of digital io pins on the chip
uint8_t  DigIO_numChannels(void){
  return PINS_NUM; // fixed maximum number of pins mapped to arduino
}

int8_t DigIO_setDirection(uint8_t pin, PinDirection dir) {
  return -1;
}

int8_t DigIO_getDirection(uint8_t pin){
  return -1;
}

int8_t DigIO_setValue(uint8_t pin, uint8_t value) {
  return -1;
}

int8_t DigIO_getValue(uint8_t pin){
  return -1;
}
