#define FCY 16000000

#include <p33FJ128MC802.h>
#include <libpic30.h>
#include <string.h>

#include "uart.h"
#include "buffer_utils.h"
#include "delay.h"

//ia oscillator initialization
//ia FOSCSEL: Oscillator Source Selection Register config
//#pragma config FNOSC = 0b011      //ia select internal oscillator source -> (XTPLL)
//                                  //ia Primary Oscillator with PLL: Medium-Frequency Mode (XTPLL)

#pragma config FNOSC = FRC      //ia select internal oscillator source -> (FRC)
                                //ia FOSC: Oscillator Configuration Register config
#pragma config FCKSM = CSECMD   //ia enable clock switching
#pragma config POSCMD = XT      //ia configure POSC for XT mode (internal crystal)

//ia oscillator initialization in order to work well with setBaud115200()
void setupOscillator(void) { 
  //ia PLL Feedback Divisor Register
  PLLFBD = 62;              // M=64
  
  //ia clock division register configuration
  CLKDIVbits.PLLPOST = 1;       //ia N1=4 -> postscaler default
  CLKDIVbits.PLLPRE = 3;        //ia N2=5 -> prescaler
  
  //ia Tune FRC oscillator, if FRC is used
  OSCTUN = 0;

  //ia write something in OSCCON: Oscillator Control Register
  __builtin_write_OSCCONH(0x03); 
  __builtin_write_OSCCONL(OSCCON | 0x01);


  //ia check whether the Current Oscillator Selection bits (read-only) is ok
  // wait for clock switch to occur
  while (OSCCONbits.COSC != 0b011); 
  //ia PLL Lock Status bit (read-only) -> wait for PLL to lock
  while (OSCCONbits.LOCK != 1);
}

void setBaud57600(void) {
#define BAUD 57600
#define FREQ_SCALE 4
  U1BRG = (FCY / (FREQ_SCALE * BAUD)) - 1;
#undef FREQ_SCALE
#undef BAUD
}

void setBaud115200(void) {
#define BAUD 115200
#define FREQ_SCALE 4
  U1BRG = (FCY / (FREQ_SCALE * BAUD)) - 1;
#undef FREQ_SCALE
#undef BAUD
}

#define UART_BUFFER_SIZE 256

typedef struct UART_ {
  int tx_buffer[UART_BUFFER_SIZE];
  volatile uint8_t tx_start;
  volatile uint8_t tx_end;
  volatile uint8_t tx_size;

  int rx_buffer[UART_BUFFER_SIZE];
  volatile uint8_t rx_start;
  volatile uint8_t rx_end;
  volatile uint8_t rx_size;

  int baud;
  int uart_num; // hardware uart;
} UART;

static UART uart_0;

struct UART* UART_init(const char* device __attribute__((unused)), uint32_t baud) {  
  
  //ia can I initialize the oscillator here?
  setupOscillator();
  
  UART* uart = &uart_0;
  uart->uart_num = 0;

  switch (baud) {
    case 57600: setBaud57600();
      break;
    case 115200: setBaud115200();
      break;
    default: return 0;
  }

  uart->tx_start = 0;
  uart->tx_end = 0;
  uart->tx_size = 0;
  uart->rx_start = 0;
  uart->rx_end = 0;
  uart->rx_size = 0;

  //ia UART control register configuration
  U1MODEbits.UARTEN = 1;  // enables RX and TX
  U1MODEbits.USIDL = 0;   // Continue in Idle
  U1MODEbits.IREN = 0;    // No IR translation
  U1MODEbits.RTSMD = 1;   // Simplex Mode
  U1MODEbits.UEN = 0;     // U1TX and U1RX enabled, CTS,RTS not -- required?
  U1MODEbits.WAKE = 0;    // No Wake up (since we don't sleep here) -- required?
  U1MODEbits.LPBACK = 0;  // No Loop Back
  U1MODEbits.ABAUD = 0;   // No Autobaud (would require sending '55')
  U1MODEbits.URXINV = 0;  // IdleState = 1  (for dsPIC)
  U1MODEbits.BRGH = 1;    // 4 clocks per bit period
  U1MODEbits.PDSEL = 0;   // mode 01: 8-bit data, even parity
  U1MODEbits.STSEL = 0;   // 1 stop bit

  //ia UART status & control register configuration
  U1STAbits.UTXINV = 0;   // U1TX Idle state is '1'
  U1STAbits.UTXBRK = 0;   // Sync Break TX Disabled
  U1STAbits.UTXBF = 0;    // TX Buffer not full, one+ more char can be written
  U1STAbits.TRMT = 0;     // TX Shift Register not full, TX in progress or queued
  U1STAbits.ADDEN = 0;    // 8-bit data, Address Detect Disabled

  //ia interrupt configuration
  IPC2bits.U1RXIP = 0x4;    // RX Mid Range Interrupt Priority level (0100 => 4), no urgent reason
  IPC3bits.U1TXIP = 0x4;    // TX Mid Range Interrupt Priority level (0100 => 4), no urgent reason
  U1STAbits.URXISEL1 = 0;   // Interrupt when any char is received and transferred from the U1RSR to the receive buffer.
  U1STAbits.URXISEL0 = 0;   // Second part of configuration
  U1STAbits.UTXISEL1 = 0;   // interrupt when char is transferred to the TSR
  U1STAbits.UTXISEL0 = 0;   // Second part of configuration

  //ia CONTROL REGISTER UNLOCK: enable writes
  asm volatile ( "mov #OSCCONL, w1 \n"
      "mov #0x45, w2 \n"
      "mov #0x57, w3 \n"
      "mov.b w2, [w1] \n"
      "mov.b w3, [w1] \n"
      "bclr OSCCON, #6 ");

  //ia @brief pin configurations:
  //ia        RP7 -> RX; RP8 -> TX; 
  RPINR18bits.U1RXR = 7;  //rx
  RPOR4bits.RP8R = 3;     //tx
  
  //ia this two register override the analog/digital thing on the pins
  //ia be careful with those, for now they are not required to be set
  AD1PCFGL = 0xFFFF; //ia sets ALL pins as digitals. 

  //ia CONTROL REGISTER LOCK: disable writes
  asm volatile ( "mov #OSCCONL, w1 \n"
      "mov #0x45, w2 \n"
      "mov #0x57, w3 \n"
      "mov.b w2, [w1] \n"
      "mov.b w3, [w1] \n"
      "bset OSCCON, #6"); 
  
  //Fire the engine
  U1STAbits.UTXEN = 1; // TX Enabled, TX pin controlled by UART
  IFS0bits.U1TXIF = 0; // Clear the Transmit Interrupt Flag
  IFS0bits.U1RXIF = 0; // Clear the Receive Interrupt Flag
  IEC0bits.U1TXIE = 1; // Enable Transmit Interrupts
  IEC0bits.U1RXIE = 1; // Enable Receive Interrupts

  return &uart_0;
}

// returns the free space in the buffer
int UART_rxbufferSize(struct UART* uart __attribute__((unused))) {
  return UART_BUFFER_SIZE;
}

// returns the free occupied space in the buffer
int UART_txBufferSize(struct UART* uart __attribute__((unused))) {
  return UART_BUFFER_SIZE;
}

// number of chars in rx buffer
int UART_rxBufferFull(UART* uart) {
  return uart->rx_size;
}

// number of chars in tx buffer
int UART_txBufferFull(UART* uart) {
  return uart->tx_size;
}

// number of free chars in tx buffer
int UART_txBufferFree(UART* uart) {
  return UART_BUFFER_SIZE - uart->tx_size;
}

void UART_putChar(struct UART* uart, uint8_t c) {
  while (uart->tx_size >= UART_BUFFER_SIZE);
  
  //ATOMIC Execution of following Code
  asm volatile ("disi #0x3FFF"); 
  uart->tx_buffer[uart->tx_end] = c;
  BUFFER_PUT(uart->tx, UART_BUFFER_SIZE);
  asm volatile ("disi #0"); 
  
  //ia rise the flag for a transmit interrupt
  IFS0bits.U1TXIF = 1;
  
  //ia required, burns a lot of cycles but otherwise it won't work
  __delay_us(100); 
}

uint8_t UART_getChar(struct UART* uart) {
  while (uart->rx_size == 0);
  
  uint8_t c;
  //ATOMIC Execution of following Code
  asm volatile ("disi #0x3FFF"); 
  c = uart->rx_buffer[uart->rx_start];
  BUFFER_GET(uart->rx, UART_BUFFER_SIZE);
  asm volatile ("disi #0"); 
  
  return c;
}

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt() {
  uint8_t c = U1RXREG;
  if (uart_0.rx_size < UART_BUFFER_SIZE) {
    uart_0.rx_buffer[uart_0.rx_end] = c;
    BUFFER_PUT(uart_0.rx, UART_BUFFER_SIZE);
  }
  
  //ia clear the receive interrupt flag
  IFS0bits.U1RXIF = 0;
}

void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt() {
  if (uart_0.tx_size) {
    U1TXREG = uart_0.tx_buffer[uart_0.tx_start];
    BUFFER_GET(uart_0.tx, UART_BUFFER_SIZE);
  } else {
    //ia clear the transmit interrupt flag
    IFS0bits.U1TXIF = 0;
  }
}
