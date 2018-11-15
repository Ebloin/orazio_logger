#define FCY 16000000UL
#include "delay.h"
#include <libpic30.h>

void delayMs(uint16_t ms __attribute__((unused))){
  while (ms) {
    __delay_ms(1);
    --ms;
  }
}