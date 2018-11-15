//----------------------------------------------------------------------------//
//------------------------ DSPIC on DSNAV board ------------------------------//
//----------------------------------------------------------------------------//
//ia @breif there are 14RB pins that can be mapped for doing things
//ia        they are used for PWMs, encoders and general digital-I/O stuff
//ia        RB7 and RB8 are missing since they are used for the UART.

//ia @brief in this implementation we have the following pin configuration:
//ia        Encoders: RP10 -> QEA1; RP11 -> QEB1; 
//ia                  RP06 -> QEA2; RP05 -> QEB2; 
//ia        PWM: RP14 -> PWM1H1; RP15 -> PWM1DIR; 
//ia             RP12 -> PWM1H2; RP13 -> PWM2DIR; 
//ia
//ia to select TRISBbits.TRISB15 we will use:
//ia c = 2
//ia const Pin* pin = pins+c;
//ia *pin->dir_register |= (1<<pin->bit)
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


#include <p33FJ128MC802.h>
#include "pins.h"

const Pin pins[] =
  {
    //0
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=0,
      .tcc_register=0,
      .oc_register=0,
      .com_mask=0
    },
    //1
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=1,
      .tcc_register=0,
      .oc_register=0,
      .com_mask=0
    },
    //2 PWM1L1 -> this is used for the direction of the PWM1
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=15,
      .tcc_register=0,
      .oc_register=0,     
//      .com_mask=1<<0        //ia enable PWM1L1 
      .com_mask=0             //ia disable PWM1L1 
    },
    //3 PWM1H1 -> actual PWM1 generator
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=14,
      .tcc_register=&PWM1CON1,
      .oc_register=&P1DC1,
      .com_mask=1<<4          //ia enable PWM1H1
    },
    //4 PWM1L2 -> this is used for the direction of the PWM2
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=13,
      .tcc_register=0,
      .oc_register=0,
//      .com_mask=1<<1        //ia enable PWM1L2
      .com_mask=0             //ia disable PWM1L2
    },
    //5 PWM1H2 -> actual PWM2 generator
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=12,
      .tcc_register=&PWM1CON1,
      .oc_register=&P1DC2,
      .com_mask=1<<5          //ia enable PWM1H2
    },
    //6 QEB1
    {
      //ia disabled the in/out/dir registers since this is used for QEI
//      .in_register=&PORTB,
//      .out_register=&LATB,
//      .dir_register=&TRISB,
      .in_register=0,
      .out_register=0,
      .dir_register=0,
      .bit=11,
      .tcc_register=0,
      .oc_register=0,
//      .com_mask=1<<2  //ia enable PWM1L3 (you have to set in/out/dir registers)
      .com_mask=0       //ia disable PWM1L3
    },
    //7 QEA1
    {
      //ia disabled the in/out/dir registers since this is used for QEI
//      .in_register=&PORTB,
//      .out_register=&LATB,
//      .dir_register=&TRISB,
      .in_register=0,
      .out_register=0,
      .dir_register=0,
      .bit=10,
      .tcc_register=0,
      .oc_register=0,
//      .com_mask=1<<6  //ia enable PWM1H3 (you have to set in/out/dir registers)
      .com_mask=0       //ia disable PWM1H3
    },
    //8 PWM (not in dspic)
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=9,
      .tcc_register=0,
      .oc_register=0,
      .com_mask=0
    },
    //9 QEA2
    {
      //ia disabled the in/out/dir registers since this is used for QEI
//      .in_register=&PORTB,
//      .out_register=&LATB,
//      .dir_register=&TRISB,
      .in_register=0,
      .out_register=0,
      .dir_register=0,
      .bit=6,
      .tcc_register=0,
      .oc_register=0,
      .com_mask=0
    },
    //10 QEB2
    {
      //ia disabled the in/out/dir registers since this is used for QEI
//      .in_register=&PORTB,
//      .out_register=&LATB,
//      .dir_register=&TRISB,
      .in_register=0,
      .out_register=0,
      .dir_register=0,
      .bit=5,
      .tcc_register=0,
      .oc_register=0,
      .com_mask=0
    },
    //11 PWM (not in dspic)
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=4,
      .tcc_register=0,
      .oc_register=0,
      .com_mask=0
    },
    //12 PWM (not in dspic)
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=3,
      .tcc_register=0,
      .oc_register=0,
      .com_mask=0
    },
    //13 PWM (not in dspic)
    {
      .in_register=&PORTB,
      .out_register=&LATB,
      .dir_register=&TRISB,
      .bit=2,
      .tcc_register=0,
      .oc_register=0,
      .com_mask=0
    }
  };
