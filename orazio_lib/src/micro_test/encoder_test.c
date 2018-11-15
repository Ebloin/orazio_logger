#include "encoder.h"
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

int main(void){
  Encoder_init();
  
  uart=UART_init("uart_0",115200);
  while(1) {
    Encoder_sample();
    char buffer[300];
    printString("encoders: [");
    for (int i=0; i<Encoder_numEncoders(); ++i){
      sprintf(buffer, " %05d ", Encoder_getValue(i));
      printString(buffer);
    }
    printString("]\n");
    delayMs(100);
  }
}
