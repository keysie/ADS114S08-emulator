# ADS114S08-emulator
Emulate an ADS114S08 A/D-converter using an Arduino Uno

## Emulated behaviour

Device tries to emulate the behaviour of the AD converter
in fully multiplexed single shot conversion mode. To achieve
this, the emulator waits for a START command on SPI. Once 
START has been received, it will simulate the conversion time
by simply waiting. Shortly before the conversion time is done,
DRDY will go HIGH and as soon as the conversion is ready it
will go back to LOW. This falling flank is the signal for the
master to clock out the two bytes of the result using NOPs.

### SPI pinout:
* SCK  : pin 13 (PB5)
* MISO : pin 12 (PB4)
* MOSI : pin 11 (PB3)
* SS   : pin 10 (PB2)
* DRDY : pin 9  (PB1)
