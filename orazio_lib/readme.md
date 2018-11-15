## SRRG2 Orazio Firmware
 Brand new firmware for the Orazio robot. Designed with modularity in mind,
 the firmware can be easily ported to different architectures without changing
 the upper layers.

In order to change architecture, the only thing that must be implemented are
the low level peripherals (like uart, digitalIO, encoders and so on) that are
contained in the ```arch``` folder.

Currently available architectures:
- ATMEGA 2560 (aka Arduino Mega)
- ATMEGA 328  (aka Arduino Uno)
- DSPIC33FMC802F (on the MuIn DSNav board)

Partially developed
- ARM Cortex M0 (on the Teensy 3.6 board)

#### Notes on DSPIC implementation
- This architecture only works with one of the possible H-Bridge implementations
available, the one with **pwm+direction**
(that corresponds to the ```mode 0``` in the firmware).
- The pin configuration is the following:
```
encoders: RP10 -> QEA1; RP11 -> QEB1; RP06 -> QEA2; RP05 -> QEB2;
pwm:      RP14 -> PWM1H1; RP15 -> PWM1DIR; RP12 -> PWM1H2; RP13 -> PWM2DIR;
uart:     RP7 -> RX; RP8 -> TX;
```
