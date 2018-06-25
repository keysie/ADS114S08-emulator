// Written by Robert Simpson
// robert_zwilling@web.de
// February 2018
// Inspired by this code: http://www.gammon.com.au/spi

// This sketch configures an Arduino Uno to emulate an ADS114S08
// A/D converter.

/* DEBUG VERSION OF CODE. LOTS OF SERIAL OUTPUT IN IT.
 *  USE ONLY (!!!) TO FIND ERRORS. USE VERSION WITHOUT
 *  SERIAL DEBUGGING PARTS FOR PRODUCTION. SERIAL 
 *  INTERFACE TAKES A LOT OF TIME, THIS SCREWS WITH
 *  SPI TIMINGS!
 */

/*
 * Device details:
 * ***************
 * 
 * - Int. Osc.: 4.096 MHz 1.5%
 * - 12 Inputs
 * - Gain 1...128
 * - Word length: 8bit
 * - MSb first
*/

/* 
 * SPI configuration: 
 * ******************
 * 
 * - CS active low
 * - fmax = 100 MHz  (more like 1 MHz on flying wires) 
 * - SPI mode = 1 --> CPOL = 0 && CPHA = 1 
 */

/*
 * Emulated behaviour:
 * *******************
 * 
 * Device tries to emulate the behaviour of the AD converter
 * in fully multiplexed single shot conversion mode. To achieve
 * this, the emulator waits for a START command on SPI. Once 
 * START has been received, it will simulate the conversion time
 * by simply waiting. Shortly before the conversion time is done,
 * DRDY will go HIGH and as soon as the conversion is ready it
 * will go back to LOW. This falling flank is the signal for the
 * master to clock out the two bytes of the result using NOPs.
 */

// SPI pinout:
// SCK  : pin 13 (PB5)
// MISO : pin 12 (PB4)
// MOSI : pin 11 (PB3)
// SS   : pin 10 (PB2)
// DRDY : pin 9  (PB1)

#include <SPI.h>

// pin definitions
#define DRDY     9

// SPI command definitions
#define SPI_NOP     0x00
#define SPI_STARTa  0x08
#define SPI_STARTb  0x09

volatile boolean started;           // 0=not converting, 1=converting
volatile boolean data_ready;        // true as soon as conversion period is done
volatile byte channel_number;       // holds the number of the currently active channel
volatile int channel_value;         // holds the current measurement
volatile int base_value[6];         // holds random base value for each channel

int tictime;
int delaytime;

void setup (void)
{
  Serial.begin (115200);   // Establish serial for debug
  Serial.write("ADS114S08 emulation starting\n");

  // initialize variablies
  started = false;
  data_ready = false;
  channel_number = 0;
  channel_value = 0;
  for (byte i = 0; i < 6; i++)
  {
    base_value[i] = random(-30000, 30000);
  }

  // prepare GPIO9 for DRDY duty
  pinMode(DRDY, OUTPUT);
  digitalWrite(DRDY, LOW);
  
  // SPI configuration
  SPI.setBitOrder(MSBFIRST);  // msb first
  pinMode(MISO, OUTPUT);      // MISO becomes output in slave config 
  SPCR |= bit (SPE);          // enable spi in slave mode

  // turn on interrupts
  SPI.attachInterrupt();

  Serial.println("Setup done");

}  // end of setup


// SPI interrupt routine
ISR (SPI_STC_vect)
{
  byte command = SPDR;  // read spi data buffer

  Serial.println("received:");
  Serial.println(command);
  
  if (command == SPI_STARTa || command == SPI_STARTb)
  {
    if (started == false)
    {
      Serial.println("setting flag");
      started = true; 
      SPDR = 0;
    }
  }
  else if (command == SPI_NOP)
  {
    Serial.println("NOP");
    if (data_ready)
    {
      /*
       * if this happens it means that the msbyte of the channel value
       * has already been clocked out at the same time that this NOP
       * command has been clocked in. Therefore now load the lsbyte of
       * the channel value into the SPI data register where it can be
       * clocked out with the next NOP, and reset the rest of the state
       * machine.
       */
       SPDR = lowByte(channel_value);
       delaytime = micros() - tictime;
       Serial.println(delaytime);
       data_ready = false;
    }
  }
      
}  // end of interrupt routine SPI_STC_vect


void loop (void)
{
  if (started)
  {
    Serial.println("started");
    
    /* 
     *  Assuming a sample rate setting of 4000 SPS and usage of the
     *  low latency filter together with single shot conversions, the
     *  expected total delay is 560us. The calculations take about 120us.
     */
    delayMicroseconds(440);
    digitalWrite(DRDY, HIGH);

    // generate the current channel measurement based on the initially
    // obtained (and now constant) base number for the channel and some
    // additive noise.
    channel_value =  base_value[channel_number] + random(-2000, 2000);

    Serial.println("channel value:");
    Serial.println(channel_value);

    // increase the channel number by one (not completely reliable, but
    // should lead to a cyclic change of the channels)
    channel_number = (channel_number + 1) % 6;

    // store the msbyte of the channel value in the SPI data register,
    // such that it can be clocked out by the next NOP from the master
    SPDR = highByte(channel_value);

    // about 560us have elapsed, therefore signal the host that conversion
    // data is now ready
    Serial.println("ready");
    started = false;
    data_ready = true;
    digitalWrite(DRDY, LOW);
    tictime = micros();
  }
    
}  // end of loop
