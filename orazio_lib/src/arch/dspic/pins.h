#include <stdint.h>

typedef struct{
  volatile uint16_t* in_register;
  volatile uint16_t* out_register;
  volatile uint16_t* dir_register;
  uint8_t bit;

  // timer registers for PWM
  volatile uint16_t* tcc_register;
  volatile uint16_t* oc_register;
  const uint16_t com_mask;

  // interrupt
}  Pin;

#define PINS_NUM 14

extern const Pin pins[];
