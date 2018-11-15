#include "encoder.h"

#define NUM_ENCODERS 2
#define ENCODER_MASK 0x0F // last four bit of PORT B used for encoders

typedef struct{
  uint16_t current_value;
  uint16_t sampled_value;
  uint8_t  pin_state;
}  Encoder;

Encoder _encoders[NUM_ENCODERS]={
  {0,0},
  {0,0}
};


// initializes the encoder subsystem
void Encoder_init(void){
}

// samples the encoders, saving the respective values in a temporary storage
void Encoder_sample(void){
}

// returns the number of the encoder 
uint8_t Encoder_numEncoders(void){
  return NUM_ENCODERS;
}

// returns the value of an encoder, when sampled with the Encoder_sample();
uint16_t Encoder_getValue(uint8_t num_encoder){
  return _encoders[num_encoder].sampled_value;
}

static const int8_t _transition_table []=
  {
      0,  //0000
      1, //0001
     -1,  //0010
      0,  //0011
     -1,  //0100
      0,  //0101
      0,  //0110
      1, //0111
      1, //1000
      0,  //1001
      0,  //1010
     -1,  //1011
      0,  //1100
     -1,  //1101
      1,  //1110
      0   //1111
    };


/* ISR(PCINT0_vect) { */
/*   cli(); */
/*   char port_value=PINB&ENCODER_MASK; */
/*   // encoder 0 */
/*   uint8_t new_pin_state=port_value&0x3; */
/*   uint8_t idx=(_encoders[0].pin_state<<2)| new_pin_state ; */
/*   _encoders[0].current_value+=_transition_table[idx]; */
/*   _encoders[0].pin_state=new_pin_state; */
/*   //encoder 1 */
/*   new_pin_state=port_value>>2; */
/*   idx=(_encoders[1].pin_state<<2)| new_pin_state ; */
/*   _encoders[1].current_value+=_transition_table[idx]; */
/*   _encoders[1].pin_state=new_pin_state; */
/*   sei(); */
/* } */
