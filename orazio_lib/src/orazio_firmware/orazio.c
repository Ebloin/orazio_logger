#include <assert.h>
#include <string.h>
#include <math.h>
#include "timer.h"
#include "eeprom.h"
#include "encoder.h"
#include "digio.h"
#include "sonar.h"
#include "pwm.h"

#include "orazio_globals.h"
#include "orazio_param.h"
#include "orazio_comm.h"
#include "orazio_joints.h"
#include "orazio_drive.h"
#include "orazio_sonar.h"
#include "orazio_watchdog.h"
#include <stdio.h>

// used to handle communication
volatile uint8_t tick_counter;           //incremented at each timer tick, reset on comm
volatile uint8_t comm_handle=0;
// this is to remember how many packets we received since the last update

void onTimerTick(void* args __attribute__((unused))){
  Orazio_jointsHandle();
  --tick_counter;
  if (!tick_counter){
    tick_counter=system_params.comm_cycles;
    comm_handle=1;
  }
}


int main(int argc, char** argv){
  // initialize devices
  EEPROM_init();
  DigIO_init();
  PWM_init();
  Encoder_init();
  Timers_init();
  // initialize subsystems
  Orazio_paramInit();
  Orazio_jointsInit();
  Orazio_driveInit();

#ifdef _ORAZIO_USE_SONAR_
  Sonar_init();
  Orazio_sonarInit();
#endif
  
  Orazio_commInit();
  //start a timer
  struct Timer* timer=Timer_create("timer_0",system_params.timer_period_ms,onTimerTick,0);
  Timer_start(timer);

  // we want to get the first data whe the platform starts
  tick_counter=system_params.comm_cycles;
  system_status.watchdog_count=system_params.watchdog_cycles;

  // loop foreva
  // remember how many packets you received before;
  
  Orazio_commSendString("Ready");
  while(1){
    ++system_status.idle_cycles; // count how long we spend doing nofin
    if (comm_handle){
      uint16_t previous_rx_packets=system_status.rx_packets;
      Orazio_driveUpdate();
      Orazio_driveControl();
#ifdef _ORAZIO_USE_SONAR_
      Orazio_sonarHandle();
#endif
      Orazio_commHandle();
      if (previous_rx_packets!=system_status.rx_packets)
        Orazio_watchdogReset();
      Orazio_watchdogHandle();
      comm_handle=0;
      system_status.idle_cycles=0;
    }
  }
}
