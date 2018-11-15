#include "orazio_globals.h"
#include "orazio_joints.h"

void Orazio_watchdogReset(void) {
  system_status.watchdog_count=system_params.watchdog_cycles;
}

void Orazio_watchdogHandle(void) {
  // watchdog disabled
  if (!system_params.watchdog_cycles){
    system_status.watchdog_count=system_params.watchdog_cycles;
    return;
  }

  // watchdog on
  if (system_status.watchdog_count>0)
    --system_status.watchdog_count;

  // reset condition
  if (!system_status.watchdog_count){
    drive_status.enabled=0;
    Orazio_jointsDisable();
  }
}
