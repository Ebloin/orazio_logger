#include "orazio_globals.h"
#include "sonar.h"
#include <stdio.h>
#include "orazio_comm.h"

uint8_t sonar_max_cycle=0;
uint8_t sonar_current_cycle=0;
uint8_t sonar_ready=0;

uint8_t Orazio_sonarReady(void) {
  return sonar_ready;
}

PacketStatus Orazio_sonarInit(void) {
  sonar_max_cycle=0;
  sonar_current_cycle=0;
  sonar_ready=0;
  
  // we need to find the maximum cycle of the sonar
  for(int s=0; s<SONARS_NUM; ++s) {
    if (sonar_max_cycle<sonar_params.pattern[s])
      sonar_max_cycle=sonar_params.pattern[s];

  }
  return Success;
}

void Orazio_sonarHandle(void){
  sonar_ready=0;
  ++sonar_current_cycle;

  if (sonar_current_cycle>sonar_max_cycle) {
    sonar_current_cycle=0;
    return;
  }


  // we construct a mask out of the potentially active sonars
  uint8_t mask=0;
  for(uint8_t s=0; s<SONARS_NUM; ++s) {
    if (!sonar_params.pattern[s])
      continue;
    if (sonar_params.pattern[s]==sonar_current_cycle){
      mask|=(1<<s);
    }
  }
  // if no sonar active, continue
  if (! mask)
    return;

  // read the previous sonars
  Sonar_get(sonar_status.ranges);
  sonar_ready=1;

  // start a new reading
  Sonar_pollPattern(&mask);
    
}
