#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include "orazio_client_test_getkey.h"

/** this stuff is to set the keyboard input as non blocking **/
static struct termios orig_termios;
void resetTerminalMode(void) {
    tcsetattr(0, TCSANOW, &orig_termios);
}

void setConioTerminalMode(void) {
    struct termios new_termios;
    tcgetattr(0, &orig_termios);
    new_termios=orig_termios;
    atexit(resetTerminalMode);
    // this disables the newline on the keyboard
    new_termios.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(0, TCSANOW, &new_termios);
}

static const char* key_map[]={
  "\x1b",            // ESC
  "\x1b\x5b\x41",    // Arrow up
  "\x1b\x5b\x42",    // Arrow down
  "\x1b\x5b\x43",    // Arrow right
  "\x1b\x5b\x44",    // Arrow left
  "s",               // system
  "r",               //ranges
  "d",               //drive
  "j",               //joints
  "x",
  " ",
  0
};

KeyCode string2keycode(char* s) {
  int idx=0;
  const char** k=key_map;
  while(*k) {
    if (!strcmp(*k, s))
      return idx;
    ++idx;
    ++k;
  }
  return KeyUnknown;
}

KeyCode getKey(){
  //we read the escape sequence in buf
  char key_buffer[10];
  int num_char=read(1,key_buffer,10);
  key_buffer[num_char]=0;
  // convert it to a key
  KeyCode key_code=string2keycode(key_buffer);
  return key_code;
}
