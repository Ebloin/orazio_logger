#include <p33FJ128MC802.h>
#include <dsp.h>
#include "encoder.h"

#define NUM_ENCODERS 2
uint16_t EncodersValue[NUM_ENCODERS] = { 0, 0 };

// initializes the encoder subsystem
void Encoder_init(void){
  //ia @brief pin configurations:
  //ia        RP10 -> QEA1; RP11 -> QEB1; 
  //ia        RP06 -> QEA2; RP05 -> QEB2; 
  RPINR14bits.QEA1R = 10;
  RPINR14bits.QEB1R = 11;
  RPINR16bits.QEA2R = 6;
  RPINR16bits.QEB2R = 5;
  
  //ia this two register override the analog/digital thing on the pins
  //ia be careful with those, for now they are not required to be set
  //ia AD1PCFGL |= 0x0038;
  
  //ia encoder 1 initialization
  QEI1CONbits.QEISIDL = 0;    //ia stop in sleep mode -> to be checked
  QEI1CONbits.SWPAB = 0;      //ia not swapping
  QEI1CONbits.PCDOUT = 0;     //ia normal io
  QEI1CONbits.POSRES = 0;     //ia index pulse does not reset position counter
  QEI1CONbits.QEIM = 0b110;   //ia QEI enabled x4 mode with counter reset by pulse index..
  QEI1CONbits.TQCS = 0;
  
  DFLT1CONbits.QEOUT = 0;     //ia enable-disable digital filter
//  DFLT1CONbits.CEID = 1;      //ia disable interrupts due to cnt errors
//  DFLT1CONbits.QECK = 0b101;  //ia 1:64 clock divide for digital filter -> maybe 1/128 or 0??
  
  POS1CNT = 0;
  
  //ia encoder 2 initialization
  QEI2CONbits.QEISIDL = 0;    //ia stop in sleep mode -> to be checked
  QEI2CONbits.SWPAB = 0;      //ia not swapping
  QEI2CONbits.PCDOUT = 0;     //ia normal io
  QEI2CONbits.POSRES = 0;     //ia index pulse does not reset position counter
  QEI2CONbits.QEIM = 0b110;   //ia QEI enabled x4 mode with counter reset by pulse index..
  QEI2CONbits.TQCS = 0;
  
  DFLT2CONbits.QEOUT = 0;     //ia enable-disable digital filter
//  DFLT2CONbits.CEID = 1;      //ia disable interrupts due to cnt errors
//  DFLT2CONbits.QECK = 0b101;  //ia 1:64 clock divide for digital filter -> maybe 1/128 or 0??
  
  POS2CNT = 0;
  //ia if you use another mode for the counter reset, 
  //ia then you have to setup those guys
  //ia MAX1CNT = 0xFFFF;
  //ia MAX2CNT = 0xFFFF;
}

//ia samples the encoders, saving the respective values in a temporary storage
void Encoder_sample(void){
  asm volatile ("disi #0x3FFF");  //ia disable all user interrupts (atomically)
    EncodersValue[0]=POS1CNT;
    EncodersValue[1]=POS2CNT;
  asm volatile ("disi #0");       //ia enable all user interrupts (atomically)
}

//ia returns the number of the encoder
uint8_t Encoder_numEncoders(void){
  return NUM_ENCODERS;
}

//ia returns the value of an encoder, when sampled with the Encoder_sample();
uint16_t Encoder_getValue(uint8_t num_encoder){
  return EncodersValue[num_encoder];
}
