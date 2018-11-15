#include "timer.h"
#include "uart.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include "sonar.h"
#include "counter.h"

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
int16_t sonar_buffer[SONARS_NUM];

int main(void){
  uart=UART_init("uart_0",115200);
  Timers_init();
  Counter_init();
  Sonar_init();
  volatile uint16_t do_stuff;
  struct Timer* timer=Timer_create("timer_0", 200, timerFn, (void*) &do_stuff); 
  Timer_start(timer);
  uint16_t tick=0;
  while(1) {
    if (do_stuff){
      uint8_t pattern=0x03;
      Sonar_get(sonar_buffer);
      Sonar_pollPattern(&pattern);
      char buffer[100];
      ++tick;
      sprintf(buffer, "counter: %05u, v:[%05u, %05u, %05u, %05u, %05u, %05u, %05u, %05u] \n ",
              Counter_get(),
              sonar_buffer[0], sonar_buffer[1], sonar_buffer[2], sonar_buffer[3],
              sonar_buffer[4], sonar_buffer[5], sonar_buffer[6], sonar_buffer[7]);
      printString(buffer);
      do_stuff=0;
    }
  }
}
