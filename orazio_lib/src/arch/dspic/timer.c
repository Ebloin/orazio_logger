#include <p33FJ128MC802.h>
#include <string.h>
#include "timer.h"

#define NUM_TIMERS 1
#define FCY 16000000

typedef struct Timer{
  int timer_num;
  uint16_t duration_ms;
  TimerFn fn;
  void* args;
} Timer;

static Timer timers[NUM_TIMERS];

//ia utilities
//ia compute time things
uint16_t computeOcrval(uint16_t duration_ms, uint8_t prescaler_set){
	uint16_t prescaler_val;
	switch (prescaler_set){
  case 0:
    prescaler_val = 1;
    break;
  case 1:
    prescaler_val = 8;
    break;
  case 2:
    prescaler_val = 64;
    break;
  case 3:
    prescaler_val = 256;
    break;
  default:
    prescaler_val = 1;
    break;
	}
  // PR3 = ocrval = FCY/prescaler*period_s
  // FCY = 16000000;  /1000 because i use [ms] and not [s]
	uint16_t ocrval = (FCY/prescaler_val/1000)*duration_ms;
	return ocrval;
}


void _timer3_start(struct Timer* timer){
  //ia configure Timer 3.
  //ia EXAMPLE: PR3 and TCKPS are set to call interrupt every 500 ms
  //ia          Period = PR3 * prescaler * Tcy = 58594 * 256 * 33.33ns = 500ms
  
  T3CON = 0;                //ia clear Timer 3 configuration
  T3CONbits.TCKPS = 3;      //ia set timer 3 prescaler (0=1:1, 1=1:8, 2=1:64, 3=1:256)
  
  IPC2bits.T3IP = 0x01;     //ia set Timer 3 interrupt priority (1)
  IFS0bits.T3IF = 0;        //ia clear Timer 3 interrupt flag
  IEC0bits.T3IE = 1;        //ia enable Timer 3 interrupt
  
  //ia set Timer 3 period (max value is 65535)
  uint16_t ocrval = computeOcrval(timer->duration_ms, T3CONbits.TCKPS); 
  PR3 = ocrval;
  
  T3CONbits.TON = 1;        //ia turn on Timer 3
}


void Timers_init(void){
  memset(timers, 0, sizeof(timers));
  for (int i=0; i<NUM_TIMERS; ++i)
    timers[i].timer_num=i;
}

//ia @brief every duration_ms the function timer_fn will be called
//ia        with arguments timer args
Timer* Timer_create(char* device,
                    uint16_t duration_ms,
                    TimerFn timer_fn,
                    void* timer_args){
  Timer* timer=0;
  if (!strcmp(device,"timer_0"))
    timer=timers;
  else
    return 0;
  timer->duration_ms=duration_ms;
  timer->timer_num=0;
  timer->fn=timer_fn;
  timer->args=timer_args;
  return timer;
}

// stops and destroys a timer
void Timer_destroy(struct Timer* timer){
  Timer_stop(timer);
  //ATOMIC Execution of following Code
  asm volatile ("disi #0x3FFF"); 
  int timer_num=timer->timer_num;
  memset(timer, 0, sizeof(Timer));
  timer->timer_num=timer_num;
  asm volatile ("disi #0x0"); 
}

//ia starts a timer
void Timer_start(struct Timer* timer){
  //ATOMIC Execution of following Code
  asm volatile ("disi #0x3FFF"); 
  if (timer->timer_num==0)
    _timer3_start(timer);
  asm volatile ("disi #0x0"); 
}

//ia stops a timer
void Timer_stop(struct Timer* timer){
  if (timer->timer_num!=0)
    return;
  
  //ATOMIC Execution of following Code
  asm volatile ("disi #0x3FFF"); 
  IEC0bits.T3IE = 0;                  // Disable Timer 3 interrupt
  asm volatile ("disi #0x0"); 
}

//ia timer ISR
void __attribute__ ((interrupt, no_auto_psv)) _T3Interrupt() {
  //clear Timer3 interrupt flag -- in atmega: TCNT5 = 0;
  IFS0bits.T3IF = 0;                  
  if(timers[0].fn)
    (*timers[0].fn)(timers[0].args);
  LATB = ~LATB;
}
