#pragma once

// resets the watchdog (call it when a wd reset condition occurs)
void Orazio_watchdogReset(void);

// handles the watchdog (call it in the main loop)
void Orazio_watchdogHandle(void);
