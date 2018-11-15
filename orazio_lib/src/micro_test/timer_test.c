#include "timer.h"
#include "uart.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>

struct UART* uart;
void printString(char* s){
  int l=strlen(s);
  for(int i=0; i<l; ++i, ++s)
    UART_putChar(uart, (uint8_t) *s);
}

void timerFn(void* args){
  uint16_t* argint=(uint16_t*)args;
  *argint=1;
}

int main(void){
  uart=UART_init("uart_0",115200);
  Timers_init();
  volatile uint16_t do_stuff;
  struct Timer* timer=Timer_create("timer_0", 100, timerFn, (void*) &do_stuff); 
  Timer_start(timer);
  uint16_t tick=0;
  while(1) {
    if (do_stuff){
      char buffer[20];
      ++tick;
      sprintf(buffer, "Tick: [ %05d]\n ", tick);
      printString(buffer);
      do_stuff=0;
    }
  }
}
