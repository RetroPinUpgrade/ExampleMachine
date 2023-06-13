/**************************************************************************
 *     This file is part of the RPU for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 3/31/2023

    RPU is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPU is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */


 
#include <Arduino.h>
#include <EEPROM.h>
//#define DEBUG_MESSAGES    1
#define RPU_CPP_FILE
#include "RPU_config.h"
#include "RPU.h"

#ifndef RPU_OS_HARDWARE_REV
#define RPU_OS_HARDWARE_REV 1
#endif

/******************************************************
 *   The board type, MPU architecture, and supported
 *   features are all controlled through the 
 *   RPU_Config.h file. 
 */
#include "RPU_Config.h"


/******************************************************
 *   Defines and library variables
 */
#if !defined(RPU_MPU_BUILD_FOR_6800) || (RPU_MPU_BUILD_FOR_6800==1)
boolean UsesM6800Processor = true;
#if (RPU_MPU_ARCHITECTURE>11) && (RPU_OS_HARDWARE_REV<102)
#error "Architecture > 11 doesn't make sense with RPU_MPU_BUILD_FOR_6800=1. Set RPU_MPU_BUILD_FOR_6800 to 0 in RPU_Config.h or choose a different RPU_MPU_ARCHITECTURE"
#endif 
#else
boolean UsesM6800Processor = false;
#endif 

#if (RPU_MPU_ARCHITECTURE<10) 

#ifdef RPU_USE_EXTENDED_SWITCHES_ON_PB4
#define NUM_SWITCH_BYTES                6
#define NUM_SWITCH_BYTES_ON_U10_PORT_A  5
#define MAX_NUM_SWITCHES                48
#define DEFAULT_SOLENOID_STATE          0x8F
#define ST5_CONTINUOUS_SOLENOID_BIT     0x10
#elif defined(RPU_USE_EXTENDED_SWITCHES_ON_PB7)
#define NUM_SWITCH_BYTES                6
#define NUM_SWITCH_BYTES_ON_U10_PORT_A  5
#define MAX_NUM_SWITCHES                48
#define DEFAULT_SOLENOID_STATE          0x1F
#define ST5_CONTINUOUS_SOLENOID_BIT     0x80
#else 
#define NUM_SWITCH_BYTES                5
#define NUM_SWITCH_BYTES_ON_U10_PORT_A  5
#define MAX_NUM_SWITCHES                40
#define DEFAULT_SOLENOID_STATE          0x9F
#endif

#if !defined(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS) || !defined(RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS)
#error "Must define RPU_OS_SWITCH_DELAY_IN_MICROSECONDS and RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS in RPU_Config.h"
#endif

#elif (RPU_MPU_ARCHITECTURE >= 10) 
#define NUM_SWITCH_BYTES              8
#define MAX_NUM_SWITCHES              64
#ifndef INTERRUPT_OCR1A_COUNTER
#define INTERRUPT_OCR1A_COUNTER         16574
#endif

volatile byte BoardLEDs = 0;
volatile boolean UpDownSwitch = false;
unsigned short ContinuousSolenoidBits = 0;

volatile byte DisplayCreditDigits[2];
volatile byte DisplayCreditDigitEnable;
volatile byte DisplayBIPDigits[2];
volatile byte DisplayBIPDigitEnable;

#if (RPU_MPU_ARCHITECTURE == 15)
volatile byte DisplayText[2][RPU_OS_NUM_DIGITS];
#endif

#endif // End of condition based on RPU_MPU_ARCHITECTURE

// Global variables
volatile byte DisplayDigits[5][RPU_OS_NUM_DIGITS];
volatile byte DisplayDigitEnable[5];
volatile boolean DisplayOffCycle = false;
volatile byte CurrentDisplayDigit=0;
volatile byte LampStates[RPU_NUM_LAMP_BANKS], LampDim1[RPU_NUM_LAMP_BANKS], LampDim2[RPU_NUM_LAMP_BANKS];
volatile byte LampFlashPeriod[RPU_MAX_LAMPS];
byte DimDivisor1 = 2;
byte DimDivisor2 = 3;

volatile byte SwitchesMinus2[NUM_SWITCH_BYTES];
volatile byte SwitchesMinus1[NUM_SWITCH_BYTES];
volatile byte SwitchesNow[NUM_SWITCH_BYTES];
#ifdef RPU_OS_USE_DIP_SWITCHES
byte DipSwitches[4];
#endif


#define SOLENOID_STACK_SIZE 150
#define SOLENOID_STACK_EMPTY 0xFF
volatile byte SolenoidStackFirst;
volatile byte SolenoidStackLast;
volatile byte SolenoidStack[SOLENOID_STACK_SIZE];
boolean SolenoidStackEnabled = true;
volatile byte CurrentSolenoidByte = 0xFF;
volatile byte RevertSolenoidBit = 0x00;
volatile byte NumCyclesBeforeRevertingSolenoidByte = 0;

#define TIMED_SOLENOID_STACK_SIZE 30
struct TimedSolenoidEntry {
  byte inUse;
  unsigned long pushTime;
  byte solenoidNumber;
  byte numPushes;
  byte disableOverride;
};
TimedSolenoidEntry TimedSolenoidStack[TIMED_SOLENOID_STACK_SIZE] = {0, 0, 0, 0, 0};

#define SWITCH_STACK_SIZE   60
#define SWITCH_STACK_EMPTY  0xFF
volatile byte SwitchStackFirst;
volatile byte SwitchStackLast;
volatile byte SwitchStack[SWITCH_STACK_SIZE];


// The WTYPE1 and WTYPE2 sound cards can only play one sound at a time,
// so these structures allow the app to send in as many calls as they
// want, but with a priority and requested amount of time to let 
// it play. This could be ported over to other architectures 
// like S&T or -51, etc., but right now I've only implemented it for
// MPU Architecture > 9
#if (RPU_MPU_ARCHITECTURE >= 10) 

#define SOUND_STACK_SIZE  64
#define SOUND_STACK_EMPTY 0x0000
volatile byte SoundStackFirst;
volatile byte SoundStackLast;
volatile unsigned short SoundStack[SOUND_STACK_SIZE];

#define TIMED_SOUND_STACK_SIZE  20
struct TimedSoundEntry {
  byte inUse;
  unsigned long pushTime;
  unsigned short soundNumber;
  byte numPushes;
};
TimedSoundEntry TimedSoundStack[TIMED_SOUND_STACK_SIZE] = {0, 0, 0, 0};
#endif

#if (RPU_OS_HARDWARE_REV==1)
#if (RPU_MPU_ARCHITECTURE!=1)
#error "RPU_OS_HARDWARE_REV 1 only works on machines with RPU_MPU_ARCHITECTURE of 1"
#endif
#define ADDRESS_U10_A           0x14
#define ADDRESS_U10_A_CONTROL   0x15
#define ADDRESS_U10_B           0x16
#define ADDRESS_U10_B_CONTROL   0x17
#define ADDRESS_U11_A           0x18
#define ADDRESS_U11_A_CONTROL   0x19
#define ADDRESS_U11_B           0x1A
#define ADDRESS_U11_B_CONTROL   0x1B
#define ADDRESS_SB100           0x10

#elif (RPU_OS_HARDWARE_REV==2)
#if (RPU_MPU_ARCHITECTURE!=1)
#error "RPU_OS_HARDWARE_REV 2 only works on machines with RPU_MPU_ARCHITECTURE of 1"
#endif
#define ADDRESS_U10_A           0x00
#define ADDRESS_U10_A_CONTROL   0x01
#define ADDRESS_U10_B           0x02
#define ADDRESS_U10_B_CONTROL   0x03
#define ADDRESS_U11_A           0x08
#define ADDRESS_U11_A_CONTROL   0x09
#define ADDRESS_U11_B           0x0A
#define ADDRESS_U11_B_CONTROL   0x0B
#define ADDRESS_SB100           0x10
#define ADDRESS_SB100_CHIMES    0x18
#define ADDRESS_SB300_SQUARE_WAVES  0x10
#define ADDRESS_SB300_ANALOG        0x18

#elif (RPU_OS_HARDWARE_REV==3) || (RPU_OS_HARDWARE_REV==4)
#if (RPU_MPU_ARCHITECTURE!=1)
#error "RPU_OS_HARDWARE_REV 3 and 4 only work on machines with RPU_MPU_ARCHITECTURE of 1"
#endif
#define ADDRESS_U10_A           0x88
#define ADDRESS_U10_A_CONTROL   0x89
#define ADDRESS_U10_B           0x8A
#define ADDRESS_U10_B_CONTROL   0x8B
#define ADDRESS_U11_A           0x90
#define ADDRESS_U11_A_CONTROL   0x91
#define ADDRESS_U11_B           0x92
#define ADDRESS_U11_B_CONTROL   0x93
#define ADDRESS_SB100           0xA0
#define ADDRESS_SB100_CHIMES    0xC0
#define ADDRESS_SB300_SQUARE_WAVES  0xA0
#define ADDRESS_SB300_ANALOG        0xC0

#elif (RPU_OS_HARDWARE_REV>=100)
#if (RPU_MPU_ARCHITECTURE<10)
#define ADDRESS_U10_A           0x88
#define ADDRESS_U10_A_CONTROL   0x89
#define ADDRESS_U10_B           0x8A
#define ADDRESS_U10_B_CONTROL   0x8B
#define ADDRESS_U11_A           0x90
#define ADDRESS_U11_A_CONTROL   0x91
#define ADDRESS_U11_B           0x92
#define ADDRESS_U11_B_CONTROL   0x93
#define ADDRESS_SB100           0xA0
#define ADDRESS_SB100_CHIMES    0xC0
#define ADDRESS_SB300_SQUARE_WAVES  0xA0
#define ADDRESS_SB300_ANALOG        0xC0
#else
#define PIA_DISPLAY_PORT_A      0x2800
#define PIA_DISPLAY_CONTROL_A   0x2801
#define PIA_DISPLAY_PORT_B      0x2802
#define PIA_DISPLAY_CONTROL_B   0x2803
#define PIA_SWITCH_PORT_A       0x3000
#define PIA_SWITCH_CONTROL_A    0x3001
#define PIA_SWITCH_PORT_B       0x3002
#define PIA_SWITCH_CONTROL_B    0x3003
#define PIA_LAMPS_PORT_A        0x2400
#define PIA_LAMPS_CONTROL_A     0x2401
#define PIA_LAMPS_PORT_B        0x2402
#define PIA_LAMPS_CONTROL_B     0x2403
#define PIA_SOLENOID_PORT_A     0x2200
#define PIA_SOLENOID_CONTROL_A  0x2201
#define PIA_SOLENOID_PORT_B     0x2202
#define PIA_SOLENOID_CONTROL_B  0x2203
#if (RPU_MPU_ARCHITECTURE==13)
#define PIA_SOUND_COMMA_PORT_A      0x2100
#define PIA_SOUND_COMMA_CONTROL_A   0x2101
#define PIA_SOUND_COMMA_PORT_B      0x2102
#define PIA_SOUND_COMMA_CONTROL_B   0x2103
#endif
#if (RPU_MPU_ARCHITECTURE==15)
#define PIA_SOUND_11_PORT_A             0x2100
#define PIA_SOUND_11_CONTROL_A          0x2101
#define PIA_SOLENOID_11_PORT_B          0x2102
#define PIA_SOLENOID_11_CONTROL_B       0x2103
#define PIA_ALPHA_DISPLAY_PORT_A        0x2C00
#define PIA_ALPHA_DISPLAY_CONTROL_A     0x2C01
#define PIA_ALPHA_DISPLAY_PORT_B        0x2C02
#define PIA_ALPHA_DISPLAY_CONTROL_B     0x2C03
#define PIA_NUM_DISPLAY_PORT_A          0x3400
#define PIA_NUM_DISPLAY_CONTROL_A       0x3401
#define PIA_WIDGET_PORT_B               0x3402
#define PIA_WIDGET_CONTROL_B            0x3403
#endif
#endif

#endif 


/******************************************************
 *   Hardware Interface Functions
 *   
 *   These functions have conditional compilation for different RPU_OS_HARDWARE_REVs
 *   
 *   RPU_OS_HARDWARE_REV 1 - Nano board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *   RPU_OS_HARDWARE_REV 2 - Nano board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *                           adds support for SB300 sound cards
 *   RPU_OS_HARDWARE_REV 3 - MEGA2560 PRO board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *                           adds support for full address space 
 *   RPU_OS_HARDWARE_REV 4 - MEGA2560 PRO board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *                           adds support for OLED display, WIFI, and multiple serial ports
 *   RPU_OS_HARDWARE_REV 4 - MEGA2560 PRO board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *                           adds support for OLED display, WIFI, and multiple serial ports
 *   RPU_OS_HARDWARE_REV 100 - MEGA2560 PRO board that plugs into processor socket (prototype)
 *   RPU_OS_HARDWARE_REV 101 - MEGA2560 PRO board that plugs into processor socket
 *                             adds support for multiple serial ports (limited release)
 *   RPU_OS_HARDWARE_REV 102 - MEGA2560 PRO board that plugs into processor socket (prototype)
 *                             adds support for OLED display, WIFI, autodetection of processor type
 *   
 */

#if (RPU_OS_HARDWARE_REV==1) or (RPU_OS_HARDWARE_REV==2)

#if defined(__AVR_ATmega2560__)
#error "ATMega requires RPU_OS_HARDWARE_REV of 3, check RPU_Config.h and adjust settings"
#endif

void RPU_DataWrite(int address, byte data) {
  
  // Set data pins to output
  // Make pins 5-7 output (and pin 3 for R/W)
  DDRD = DDRD | 0xE8;
  // Make pins 8-12 output
  DDRB = DDRB | 0x1F;

  // Set R/W to LOW
  PORTD = (PORTD & 0xF7);

  // Put data on pins
  // Put lower three bits on 5-7
  PORTD = (PORTD&0x1F) | ((data&0x07)<<5);
  // Put upper five bits on 8-12
  PORTB = (PORTB&0xE0) | (data>>3);

  // Set up address lines
  PORTC = (PORTC & 0xE0) | address;

  // Wait for a falling edge of the clock
  while((PIND & 0x10));

  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTC = PORTC | 0x20;
  
  // Wait while clock is low
  while(!(PIND & 0x10));
  
  // Wait while clock is high
  while((PIND & 0x10));

  // Wait while clock is low
  while(!(PIND & 0x10));

  // Set VMA OFF
  PORTC = PORTC & 0xDF;

  // Unset address lines
  PORTC = PORTC & 0xE0;
  
  // Set R/W back to HIGH
  PORTD = (PORTD | 0x08);

  // Set data pins to input
  // Make pins 5-7 input
  DDRD = DDRD & 0x1F;
  // Make pins 8-12 input
  DDRB = DDRB & 0xE0;
}



byte RPU_DataRead(int address) {
  
  // Set data pins to input
  // Make pins 5-7 input
  DDRD = DDRD & 0x1F;
  // Make pins 8-12 input
  DDRB = DDRB & 0xE0;

  // Set R/W to HIGH
  DDRD = DDRD | 0x08;
  PORTD = (PORTD | 0x08);

  // Set up address lines
  PORTC = (PORTC & 0xE0) | address;

  // Wait for a falling edge of the clock
  while((PIND & 0x10));

  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTC = PORTC | 0x20;

  // Wait a full clock cycle to make sure data lines are ready
  // (important for faster clocks)
  // Wait while clock is low
  while(!(PIND & 0x10));

  // Wait for a falling edge of the clock
  while((PIND & 0x10));
  
  // Wait while clock is low
  while(!(PIND & 0x10));

  byte inputData = (PIND>>5) | (PINB<<3);

  // Set VMA OFF
  PORTC = PORTC & 0xDF;

  // Wait for a falling edge of the clock
// Doesn't seem to help  while((PIND & 0x10));

  // Set R/W to LOW
  PORTD = (PORTD & 0xF7);

  // Clear address lines
  PORTC = (PORTC & 0xE0);

  return inputData;
}


void WaitClockCycle(int numCycles=1) {
  for (int count=0; count<numCycles; count++) {
    // Wait while clock is low
    while(!(PIND & 0x10));
  
    // Wait for a falling edge of the clock
    while((PIND & 0x10));
  }
}

#elif (RPU_OS_HARDWARE_REV==3)

// Rev 3 connections
// Pin D2 = IRQ
// Pin D3 = CLOCK
// Pin D4 = VMA
// Pin D5 = R/W
// Pin D6-12 = D0-D6
// Pin D13 = SWITCH
// Pin D14 = HALT
// Pin D15 = D7
// Pin D16-30 = A0-A14

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 3 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif


void RPU_DataWrite(int address, byte data) {
  
  // Set data pins to output
  DDRH = DDRH | 0x78;
  DDRB = DDRB | 0x70;
  DDRJ = DDRJ | 0x01;

  // Set R/W to LOW
  PORTE = (PORTE & 0xF7);

  // Put data on pins
  // Lower Nibble goes on PortH3 through H6
  PORTH = (PORTH&0x87) | ((data&0x0F)<<3);
  // Bits 4-6 go on PortB4 through B6
  PORTB = (PORTB&0x8F) | ((data&0x70));
  // Bit 7 goes on PortJ0
  PORTJ = (PORTJ&0xFE) | (data>>7);  

  // Set up address lines
  PORTH = (PORTH & 0xFC) | ((address & 0x0001)<<1) | ((address & 0x0002)>>1); // A0-A1
  PORTD = (PORTD & 0xF0) | ((address & 0x0004)<<1) | ((address & 0x0008)>>1) | ((address & 0x0010)>>3) | ((address & 0x0020)>>5); // A2-A5
  PORTA = ((address & 0x3FC0)>>6); // A6-A13
  PORTC = (PORTC & 0x3F) | ((address & 0x4000)>>7) | ((address & 0x8000)>>9); // A14-A15

  // Wait for a falling edge of the clock
  while((PINE & 0x20));

  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTG = PORTG | 0x20;

  // Wait while clock is low
  while(!(PINE & 0x20));

  // Wait while clock is high
  while((PINE & 0x20));

  // Wait while clock is low
  while(!(PINE & 0x20));

  // Set VMA OFF
  PORTG = PORTG & 0xDF;

  // Unset address lines
  PORTH = (PORTH & 0xFC);
  PORTD = (PORTD & 0xF0);
  PORTA = 0;
  PORTC = (PORTC & 0x3F);
  
  // Set R/W back to HIGH
  PORTE = (PORTE | 0x08);

  // Set data pins to input
  DDRH = DDRH & 0x87;
  DDRB = DDRB & 0x8F;
  DDRJ = DDRJ & 0xFE;
  
}



byte RPU_DataRead(int address) {
  
  // Set data pins to input
  DDRH = DDRH & 0x87;
  DDRB = DDRB & 0x8F;
  DDRJ = DDRJ & 0xFE;

  // Set R/W to HIGH
  DDRE = DDRE | 0x08;
  PORTE = (PORTE | 0x08);

  // Set up address lines
  PORTH = (PORTH & 0xFC) | ((address & 0x0001)<<1) | ((address & 0x0002)>>1); // A0-A1
  PORTD = (PORTD & 0xF0) | ((address & 0x0004)<<1) | ((address & 0x0008)>>1) | ((address & 0x0010)>>3) | ((address & 0x0020)>>5); // A2-A5
  PORTA = ((address & 0x3FC0)>>6); // A6-A13
  PORTC = (PORTC & 0x3F) | ((address & 0x4000)>>7) | ((address & 0x8000)>>9); // A14-A15

  // Wait for a falling edge of the clock
  while((PINE & 0x20));

  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTG = PORTG | 0x20;

  // Wait a full clock cycle to make sure data lines are ready
  // (important for faster clocks)
  // Wait while clock is low
  while(!(PINE & 0x20));

  // Wait for a falling edge of the clock
  while((PINE & 0x20));
  
  // Wait while clock is low
  while(!(PINE & 0x20));

  byte inputData;
  inputData = (PINH & 0x78)>>3;
  inputData |= (PINB & 0x70);
  inputData |= PINJ << 7;

  // Set VMA OFF
  PORTG = PORTG & 0xDF;

  // Set R/W to LOW
  PORTE = (PORTE & 0xF7);

  // Unset address lines
  PORTH = (PORTH & 0xFC);
  PORTD = (PORTD & 0xF0);
  PORTA = 0;
  PORTC = (PORTC & 0x3F);

  return inputData;
}


void WaitClockCycle(int numCycles=1) {
  for (int count=0; count<numCycles; count++) {
    // Wait while clock is low
    while(!(PINE & 0x20));
  
    // Wait for a falling edge of the clock
    while((PINE & 0x20));
  }
}

#elif (RPU_OS_HARDWARE_REV==4)

// Rev 3 connections
// Pin D2 = IRQ
// Pin D3 = CLOCK
// Pin D4 = VMA
// Pin D5 = R/W
// Pin D6-12 = D0-D6
// Pin D13 = SWITCH
// Pin D14 = HALT
// Pin D15 = D7
// Pin D16-30 = A0-A14

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 4 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN                   40
#define RPU_RW_PIN                    3
#define RPU_PHI2_PIN                  39
#define RPU_SWITCH_PIN                38
#define RPU_BUFFER_DISABLE            5
#define RPU_HALT_PIN                  41
#define RPU_RESET_PIN                 42
#define RPU_DIAGNOSTIC_PIN            44
#define RPU_PINS_OUTPUT true
#define RPU_PINS_INPUT false

void RPU_SetAddressPinsDirection(boolean pinsOutput) {  
  for (int count=0; count<16; count++) {
    pinMode(A0+count, pinsOutput?OUTPUT:INPUT);
  }
}

void RPU_SetDataPinsDirection(boolean pinsOutput) {
  for (int count=0; count<8; count++) {
    pinMode(22, pinsOutput?OUTPUT:INPUT);
  }
}


// REVISION 4 HARDWARE
void RPU_DataWrite(int address, byte data) {
  
  // Set data pins to output
  DDRA = 0xFF;

  // Set R/W to LOW
  PORTE = (PORTE & 0xDF);

  // Put data on pins
  PORTA = data;

  // Set up address lines
  PORTF = (byte)(address & 0x00FF);
  PORTK = (byte)(address/256);

  if (UsesM6800Processor) {
    // Wait for a falling edge of the clock
    while((PING & 0x04));
  } else {
    // Set clock low (PG2) (if 6802/8)
    PORTG &= ~0x04;
  }
  
  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTG = PORTG | 0x02;

  if (UsesM6800Processor) {
    // Wait while clock is low
    while(!(PING & 0x04));
  
    // Wait while clock is high
    while((PING & 0x04));
  
    // Wait while clock is low
    while(!(PING & 0x04));  
  } else {
    // Set clock high
    PORTG |= 0x04;
  
    // Set clock low
    PORTG &= ~0x04;
  
    // Set clock high
    PORTG |= 0x04;
  }

  // Set VMA OFF
  PORTG = PORTG & 0xFD;

  // Unset address lines
  PORTF = 0x00;
  PORTK = 0x00;
  
  // Set R/W back to HIGH
  PORTE = (PORTE | 0x20);

  // Set data pins to input
  DDRA = 0x00;
  
}


byte RPU_DataRead(int address) {
  
  // Set data pins to input
  DDRA = 0x00;

  // Set R/W to HIGH
  DDRE = DDRE | 0x20;
  PORTE = (PORTE | 0x20);

  // Set up address lines
  PORTF = (byte)(address & 0x00FF);
  PORTK = (byte)(address/256);

  if (UsesM6800Processor) {
    // Wait for a falling edge of the clock
    while((PING & 0x04));
  } else {
    // Set clock low
    PORTG &= ~0x04;
  }
  
  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTG = PORTG | 0x02;

  if (UsesM6800Processor) {
    // Wait a full clock cycle to make sure data lines are ready
    // (important for faster clocks)
    // Wait while clock is low
    while(!(PING & 0x04));
  
    // Wait for a falling edge of the clock
    while((PING & 0x04));
    
    // Wait while clock is low
    while(!(PING & 0x04));
  } else {
    // Set clock high
    PORTG |= 0x04;
  
    // Set clock low
    PORTG &= ~0x04;
    
    // Set clock high
    PORTG |= 0x04;
  }
  
  byte inputData;
  inputData = PINA;

  // Set VMA OFF
  PORTG = PORTG & 0xFD;

  // Set R/W to LOW
  PORTE = (PORTE & 0xDF);

  // Unset address lines
  PORTF = 0x00;
  PORTK = 0x00;

  return inputData;
}

#elif (RPU_OS_HARDWARE_REV==100)

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN         4
#define RPU_RW_PIN          5
#define RPU_PHI2_PIN        3
#define RPU_SWITCH_PIN      13
#define RPU_BUFFER_DISABLE  2
#define RPU_HALT_PIN        14
#define RPU_RESET_PIN       14
#define RPU_PINS_OUTPUT true
#define RPU_PINS_INPUT false

void RPU_SetAddressPinsDirection(boolean pinsOutput) {  
  for (int count=0; count<16; count++) {
    pinMode(16+count, pinsOutput?OUTPUT:INPUT);
  }
}

void RPU_SetDataPinsDirection(boolean pinsOutput) {
  for (int count=0; count<7; count++) {
    pinMode(6+count, pinsOutput?OUTPUT:INPUT);
  }
  pinMode(15, pinsOutput?OUTPUT:INPUT);
}


// REV 100 HARDWARE
void RPU_DataWrite(int address, byte data) {
  
  // Set data pins to output
  DDRH = DDRH | 0x78;
  DDRB = DDRB | 0x70;
  DDRJ = DDRJ | 0x01;

  // Set R/W to LOW
  PORTE = (PORTE & 0xF7);

  // Put data on pins
  // Lower Nibble goes on PortH3 through H6
  PORTH = (PORTH&0x87) | ((data&0x0F)<<3);
  // Bits 4-6 go on PortB4 through B6
  PORTB = (PORTB&0x8F) | ((data&0x70));
  // Bit 7 goes on PortJ0
  PORTJ = (PORTJ&0xFE) | (data>>7);  

  // Set up address lines
  PORTH = (PORTH & 0xFC) | ((address & 0x0001)<<1) | ((address & 0x0002)>>1); // A0-A1
  PORTD = (PORTD & 0xF0) | ((address & 0x0004)<<1) | ((address & 0x0008)>>1) | ((address & 0x0010)>>3) | ((address & 0x0020)>>5); // A2-A5
  PORTA = ((address & 0x3FC0)>>6); // A6-A13
  PORTC = (PORTC & 0x3F) | ((address & 0x4000)>>7) | ((address & 0x8000)>>9); // A14-A15

  // Set clock low
  PORTE &= ~0x20;

  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTG = PORTG | 0x20;

  // Set clock high
  PORTE |= 0x20;

  // Set clock low
  PORTE &= ~0x20;

  // Set clock high
  PORTE |= 0x20;

  // Set VMA OFF
  PORTG = PORTG & 0xDF;

  // Unset address lines
  PORTH = (PORTH & 0xFC);
  PORTD = (PORTD & 0xF0);
  PORTA = 0;
  PORTC = (PORTC & 0x3F);
  
  // Set R/W back to HIGH
  PORTE = (PORTE | 0x08);

  // Set data pins to input
  DDRH = DDRH & 0x87;
  DDRB = DDRB & 0x8F;
  DDRJ = DDRJ & 0xFE;
  
}



byte RPU_DataRead(int address) {
  
  // Set data pins to input
  DDRH = DDRH & 0x87;
  DDRB = DDRB & 0x8F;
  DDRJ = DDRJ & 0xFE;

  // Set R/W to HIGH
  DDRE = DDRE | 0x08;
  PORTE = (PORTE | 0x08);

  // Set up address lines
  PORTH = (PORTH & 0xFC) | ((address & 0x0001)<<1) | ((address & 0x0002)>>1); // A0-A1
  PORTD = (PORTD & 0xF0) | ((address & 0x0004)<<1) | ((address & 0x0008)>>1) | ((address & 0x0010)>>3) | ((address & 0x0020)>>5); // A2-A5
  PORTA = ((address & 0x3FC0)>>6); // A6-A13
  PORTC = (PORTC & 0x3F) | ((address & 0x4000)>>7) | ((address & 0x8000)>>9); // A14-A15

  // Set clock low
  PORTE &= ~0x20;

  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTG = PORTG | 0x20;

  // Set clock high
  PORTE |= 0x20;

  // Set clock low
  PORTE &= ~0x20;
  
  // Set clock high
  PORTE |= 0x20;

  byte inputData;
  inputData = (PINH & 0x78)>>3;
  inputData |= (PINB & 0x70);
  inputData |= PINJ << 7;

  // Set VMA OFF
  PORTG = PORTG & 0xDF;

  // Set R/W to LOW
  PORTE = (PORTE & 0xF7);

  // Unset address lines
  PORTH = (PORTH & 0xFC);
  PORTD = (PORTD & 0xF0);
  PORTA = 0;
  PORTC = (PORTC & 0x3F);

  return inputData;
}


void WaitClockCycle(int numCycles=1) {
  for (int count=0; count<numCycles; count++) {
    // Wait while clock is low
    while(!(PINE & 0x20));
  
    // Wait for a falling edge of the clock
    while((PINE & 0x20));
  }
}


#elif (RPU_OS_HARDWARE_REV==101) || (RPU_OS_HARDWARE_REV==102)

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV >100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN           40
#define RPU_RW_PIN            3
#define RPU_PHI2_PIN          39
#define RPU_SWITCH_PIN        38
#define RPU_BUFFER_DISABLE    5
#define RPU_HALT_PIN          41
#define RPU_RESET_PIN         42
#define RPU_DIAGNOSTIC_PIN    44
#define RPU_DISABLE_PHI_FROM_MPU      7
#define RPU_DISABLE_PHI_FROM_CPU      6
#define RPU_BOARD_SEL_0               30
#define RPU_BOARD_SEL_1               31
#define RPU_BOARD_SEL_2               32
#define RPU_BOARD_SEL_3               33
#define RPU_PINS_OUTPUT       true
#define RPU_PINS_INPUT        false

void RPU_SetAddressPinsDirection(boolean pinsOutput) {  
  for (int count=0; count<16; count++) {
    pinMode(A0+count, pinsOutput?OUTPUT:INPUT);
  }
}

void RPU_SetDataPinsDirection(boolean pinsOutput) {
  for (int count=0; count<8; count++) {
    pinMode(22, pinsOutput?OUTPUT:INPUT);
  }
}


// REVISION 101 HARDWARE
void RPU_DataWrite(int address, byte data) {
  
  // Set data pins to output
  DDRA = 0xFF;

  // Set R/W to LOW
  PORTE = (PORTE & 0xDF);

  // Put data on pins
  PORTA = data;

  // Set up address lines
  PORTF = (byte)(address & 0x00FF);
  PORTK = (byte)(address/256);

  if (UsesM6800Processor) {
    // Wait for a falling edge of the clock
    while((PING & 0x04));
  } else {
    // Set clock low (PG2) (if 6802/8)
    PORTG &= ~0x04;
  }
  
  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTG = PORTG | 0x02;

  if (UsesM6800Processor) {
    // Wait while clock is low
    while(!(PING & 0x04));
  
    // Wait while clock is high
    while((PING & 0x04));
  
    // Wait while clock is low
    while(!(PING & 0x04));  
  } else {
    // Set clock high
    PORTG |= 0x04;
  
    // Set clock low
    PORTG &= ~0x04;
  
    // Set clock high
    PORTG |= 0x04;
  }

  // Set VMA OFF
  PORTG = PORTG & 0xFD;

  // Unset address lines
  PORTF = 0x00;
  PORTK = 0x00;
  
  // Set R/W back to HIGH
  PORTE = (PORTE | 0x20);

  // Set data pins to input
  DDRA = 0x00;
  
}



byte RPU_DataRead(int address) {
  
  // Set data pins to input
  DDRA = 0x00;

  // Set R/W to HIGH
  DDRE = DDRE | 0x20;
  PORTE = (PORTE | 0x20);

  // Set up address lines
  PORTF = (byte)(address & 0x00FF);
  PORTK = (byte)(address/256);

  if (UsesM6800Processor) {
    // Wait for a falling edge of the clock
    while((PING & 0x04));
  } else {
    // Set clock low
    PORTG &= ~0x04;
  }
  
  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTG = PORTG | 0x02;

  if (UsesM6800Processor) {
    // Wait a full clock cycle to make sure data lines are ready
    // (important for faster clocks)
    // Wait while clock is low
    while(!(PING & 0x04));
  
    // Wait for a falling edge of the clock
    while((PING & 0x04));
    
    // Wait while clock is low
    while(!(PING & 0x04));
  } else {
    // Set clock high
    PORTG |= 0x04;
  
    // Set clock low
    PORTG &= ~0x04;
    
    // Set clock high
    PORTG |= 0x04;
  }

  byte inputData;
  inputData = PINA;

  // Set VMA OFF
  PORTG = PORTG & 0xFD;

  // Set R/W to LOW
  PORTE = (PORTE & 0xDF);

  // Unset address lines
  PORTF = 0x00;
  PORTK = 0x00;

  return inputData;
}


#else
#error "RPU Hardware Definition Not Recognized"
#endif


#if (RPU_MPU_ARCHITECTURE<10)

void TestLightOn() {
  RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
}

void TestLightOff() {
  RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
}



void InitializeU10PIA() {
  // CA1 - Self Test Switch
  // CB1 - zero crossing detector
  // CA2 - NOR'd with display latch strobe
  // CB2 - lamp strobe 1
  // PA0-7 - output for switch bank, lamps, and BCD
  // PB0-7 - switch returns

  RPU_DataWrite(ADDRESS_U10_A_CONTROL, 0x38);
  // Set up U10A as output
  RPU_DataWrite(ADDRESS_U10_A, 0xFF);
  // Set bit 3 to write data
  RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL)|0x04);
  // Store F0 in U10A Output
  RPU_DataWrite(ADDRESS_U10_A, 0xF0);
  
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x33);
  // Set up U10B as input
  RPU_DataWrite(ADDRESS_U10_B, 0x00);
  // Set bit 3 so future reads will read data
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL)|0x04);

}

#ifdef RPU_OS_USE_DIP_SWITCHES
void ReadDipSwitches() {
  byte backupU10A = RPU_DataRead(ADDRESS_U10_A);
  byte backupU10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);

  // Turn on Switch strobe 5 & Read Switches
  RPU_DataWrite(ADDRESS_U10_A, 0x20);
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
  // Wait for switch capacitors to charge
  delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
  DipSwitches[0] = RPU_DataRead(ADDRESS_U10_B);
 
  // Turn on Switch strobe 6 & Read Switches
  RPU_DataWrite(ADDRESS_U10_A, 0x40);
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
  // Wait for switch capacitors to charge
  delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
  DipSwitches[1] = RPU_DataRead(ADDRESS_U10_B);

  // Turn on Switch strobe 7 & Read Switches
  RPU_DataWrite(ADDRESS_U10_A, 0x80);
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
  // Wait for switch capacitors to charge
  delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
  DipSwitches[2] = RPU_DataRead(ADDRESS_U10_B);

  // Turn on U10 CB2 (strobe 8) and read switches
  RPU_DataWrite(ADDRESS_U10_A, 0x00);
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl | 0x08);
  // Wait for switch capacitors to charge
  delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
  DipSwitches[3] = RPU_DataRead(ADDRESS_U10_B);

  RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl);
  RPU_DataWrite(ADDRESS_U10_A, backupU10A);
}
#endif

void InitializeU11PIA() {
  // CA1 - Display interrupt generator
  // CB1 - test connector pin 32
  // CA2 - lamp strobe 2
  // CB2 - solenoid bank select
  // PA0-7 - display digit enable
  // PB0-7 - solenoid data

  RPU_DataWrite(ADDRESS_U11_A_CONTROL, 0x31);
  // Set up U11A as output
  RPU_DataWrite(ADDRESS_U11_A, 0xFF);
  // Set bit 3 to write data
  RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL)|0x04);
  // Store 00 in U11A Output
  RPU_DataWrite(ADDRESS_U11_A, 0x00);
  
  RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x30);
  // Set up U11B as output
  RPU_DataWrite(ADDRESS_U11_B, 0xFF);
  // Set bit 3 so future reads will read data
  RPU_DataWrite(ADDRESS_U11_B_CONTROL, RPU_DataRead(ADDRESS_U11_B_CONTROL)|0x04);
  // Store 9F in U11B Output
  RPU_DataWrite(ADDRESS_U11_B, DEFAULT_SOLENOID_STATE);
  CurrentSolenoidByte = DEFAULT_SOLENOID_STATE;
  
}


unsigned long RPU_TestPIAs() {
  unsigned long piaErrors = 0;
  
  byte piaResult = RPU_DataRead(ADDRESS_U10_A_CONTROL);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_U10_PIA_ERROR;
  piaResult = RPU_DataRead(ADDRESS_U10_B_CONTROL);
  if (piaResult!=0x37) piaErrors |= RPU_RET_U10_PIA_ERROR;

  piaResult = RPU_DataRead(ADDRESS_U11_A_CONTROL);
  if (piaResult!=0x35) piaErrors |= RPU_RET_U11_PIA_ERROR;
  piaResult = RPU_DataRead(ADDRESS_U11_B_CONTROL);
  if (piaResult!=0x34) piaErrors |= RPU_RET_U11_PIA_ERROR;
  
  return piaErrors;
}

#else

void RPU_InitializePIAs() {
  RPU_DataWrite(PIA_DISPLAY_CONTROL_A, 0x31);
  RPU_DataWrite(PIA_DISPLAY_PORT_A, 0xFF);
  RPU_DataWrite(PIA_DISPLAY_CONTROL_A, 0x3D);
  RPU_DataWrite(PIA_DISPLAY_PORT_A, 0xC0);

  RPU_DataWrite(PIA_DISPLAY_CONTROL_B, 0x31);
  RPU_DataWrite(PIA_DISPLAY_PORT_B, 0xFF);
  RPU_DataWrite(PIA_DISPLAY_CONTROL_B, 0x3D);
  RPU_DataWrite(PIA_DISPLAY_PORT_B, 0x00);

  RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x38);
  RPU_DataWrite(PIA_SWITCH_PORT_A, 0x00);
  RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x3C);

  RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x38);
  RPU_DataWrite(PIA_SWITCH_PORT_B, 0xFF);
  RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x3C);
  RPU_DataWrite(PIA_SWITCH_PORT_B, 0x00);

  RPU_DataWrite(PIA_LAMPS_CONTROL_A, 0x38);
  RPU_DataWrite(PIA_LAMPS_PORT_A, 0xFF);
  RPU_DataWrite(PIA_LAMPS_CONTROL_A, 0x3C);
  RPU_DataWrite(PIA_LAMPS_PORT_A, 0xFF);  

  RPU_DataWrite(PIA_LAMPS_CONTROL_B, 0x38);
  RPU_DataWrite(PIA_LAMPS_PORT_B, 0xFF);
  RPU_DataWrite(PIA_LAMPS_CONTROL_B, 0x3C);
  RPU_DataWrite(PIA_LAMPS_PORT_B, 0x00);

#if (RPU_MPU_ARCHITECTURE<15)
  RPU_DataWrite(PIA_SOLENOID_CONTROL_A, 0x38);
  RPU_DataWrite(PIA_SOLENOID_PORT_A, 0xFF);
  RPU_DataWrite(PIA_SOLENOID_CONTROL_A, 0x3C);
#endif
  RPU_DataWrite(PIA_SOLENOID_PORT_A, 0x00);

#if (RPU_MPU_ARCHITECTURE<15)
  RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x30);
  RPU_DataWrite(PIA_SOLENOID_PORT_B, 0xFF);
  RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x34);
  RPU_DataWrite(PIA_SOLENOID_PORT_B, 0x00);
#endif

#if (RPU_MPU_ARCHITECTURE==15)
  RPU_DataWrite(PIA_SOLENOID_11_CONTROL_B, 0x38);
  RPU_DataWrite(PIA_SOLENOID_11_PORT_B, 0xFF);
  RPU_DataWrite(PIA_SOLENOID_11_CONTROL_B, 0x3C);
  RPU_DataWrite(PIA_SOLENOID_11_PORT_B, 0x00);

  RPU_DataWrite(PIA_ALPHA_DISPLAY_CONTROL_A, 0x38);
  RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_A, 0xFF);
  RPU_DataWrite(PIA_ALPHA_DISPLAY_CONTROL_A, 0x3C);
  RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_A, 0x00);

  RPU_DataWrite(PIA_ALPHA_DISPLAY_CONTROL_B, 0x38);
  RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_B, 0xFF);
  RPU_DataWrite(PIA_ALPHA_DISPLAY_CONTROL_B, 0x3C);
  RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_B, 0x00);

  RPU_DataWrite(PIA_NUM_DISPLAY_CONTROL_A, 0x38);
  RPU_DataWrite(PIA_NUM_DISPLAY_PORT_A, 0xFF);
  RPU_DataWrite(PIA_NUM_DISPLAY_CONTROL_A, 0x3C);
  RPU_DataWrite(PIA_NUM_DISPLAY_PORT_A, 0x00);

  RPU_DataWrite(PIA_SOUND_11_CONTROL_A, 0x38);
  RPU_DataWrite(PIA_SOUND_11_PORT_A, 0xFF);
  RPU_DataWrite(PIA_SOUND_11_CONTROL_A, 0x3C);
  RPU_DataWrite(PIA_SOUND_11_PORT_A, 0x00);

  RPU_DataWrite(PIA_WIDGET_CONTROL_B, 0x38);
  RPU_DataWrite(PIA_WIDGET_PORT_B, 0xFF);
  RPU_DataWrite(PIA_WIDGET_CONTROL_B, 0x3C);
  RPU_DataWrite(PIA_WIDGET_PORT_B, 0x00);
#endif

#if (RPU_MPU_ARCHITECTURE==13)
  RPU_DataWrite(PIA_SOUND_COMMA_CONTROL_A, 0x38);
  RPU_DataWrite(PIA_SOUND_COMMA_PORT_A, 0xFF);
  RPU_DataWrite(PIA_SOUND_COMMA_CONTROL_A, 0x3C);
  RPU_DataWrite(PIA_SOUND_COMMA_PORT_A, 0x00);  

  RPU_DataWrite(PIA_SOUND_COMMA_CONTROL_B, 0x38);
  RPU_DataWrite(PIA_SOUND_COMMA_PORT_B, 0xFF);
  RPU_DataWrite(PIA_SOUND_COMMA_CONTROL_B, 0x3C);
  RPU_DataWrite(PIA_SOUND_COMMA_PORT_B, 0x00);
#endif

}


unsigned long RPU_TestPIAs() {
  unsigned long piaErrors = 0;
  
  byte piaResult = RPU_DataRead(PIA_DISPLAY_CONTROL_A);
  if (piaResult!=0x3D) piaErrors |= RPU_RET_PIA_1_ERROR;
  piaResult = RPU_DataRead(PIA_DISPLAY_CONTROL_B);
  if (piaResult!=0x3D) piaErrors |= RPU_RET_PIA_1_ERROR;

  piaResult = RPU_DataRead(PIA_SWITCH_CONTROL_A);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_PIA_2_ERROR;
  piaResult = RPU_DataRead(PIA_SWITCH_CONTROL_B);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_PIA_2_ERROR;
  
  piaResult = RPU_DataRead(PIA_LAMPS_CONTROL_A);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_PIA_3_ERROR;
  piaResult = RPU_DataRead(PIA_LAMPS_CONTROL_B);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_PIA_3_ERROR;

  piaResult = RPU_DataRead(PIA_SOLENOID_CONTROL_A);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_PIA_4_ERROR;
  piaResult = RPU_DataRead(PIA_SOLENOID_CONTROL_B);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_PIA_4_ERROR;

#if (RPU_MPU_ARCHITECTURE==13)
  piaResult = RPU_DataRead(PIA_SOUND_COMMA_CONTROL_A);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_PIA_5_ERROR;
  piaResult = RPU_DataRead(PIA_SOUND_COMMA_CONTROL_B);
  if (piaResult!=0x3C) piaErrors |= RPU_RET_PIA_5_ERROR;
#endif

  return piaErrors;
}

void RPU_SetBoardLEDs(boolean LED1, boolean LED2, byte BCDValue) {
  BoardLEDs = 0;
  if (BCDValue==0xFF) {
    if (LED1) BoardLEDs |= 0x20;
    if (LED2) BoardLEDs |= 0x10;
  } else {
    BoardLEDs = BCDValue * 16;
  }
}

#endif



/******************************************************
 *   Switch Handling Functions
 */

int SpaceLeftOnSwitchStack() {
  if (SwitchStackFirst>=SWITCH_STACK_SIZE || SwitchStackLast>=SWITCH_STACK_SIZE) return 0;
  if (SwitchStackLast>=SwitchStackFirst) return ((SWITCH_STACK_SIZE-1) - (SwitchStackLast-SwitchStackFirst));
  return (SwitchStackFirst - SwitchStackLast) - 1;
}

void PushToSwitchStack(byte switchNumber) {
  //if ((switchNumber>=MAX_NUM_SWITCHES && switchNumber!=SW_SELF_TEST_SWITCH)) return;
  if (switchNumber==SWITCH_STACK_EMPTY) return;

  // If the switch stack last index is out of range, then it's an error - return
  if (SpaceLeftOnSwitchStack()==0) return;

  // Self test is a special case - there's no good way to debounce it
  // so if it's already first on the stack, ignore it
  if (switchNumber==SW_SELF_TEST_SWITCH) {
    if (SwitchStackLast!=SwitchStackFirst && SwitchStack[SwitchStackFirst]==SW_SELF_TEST_SWITCH) return;
  }

  SwitchStack[SwitchStackLast] = switchNumber;
  
  SwitchStackLast += 1;
  if (SwitchStackLast==SWITCH_STACK_SIZE) {
    // If the end index is off the end, then wrap
    SwitchStackLast = 0;
  }
}

void RPU_PushToSwitchStack(byte switchNumber) {
  PushToSwitchStack(switchNumber);
}


byte RPU_PullFirstFromSwitchStack() {
  // If first and last are equal, there's nothing on the stack
  if (SwitchStackFirst==SwitchStackLast) return SWITCH_STACK_EMPTY;

  byte retVal = SwitchStack[SwitchStackFirst];

  SwitchStackFirst += 1;
  if (SwitchStackFirst>=SWITCH_STACK_SIZE) SwitchStackFirst = 0;

  return retVal;
}


boolean RPU_ReadSingleSwitchState(byte switchNum) {
  if (switchNum>=MAX_NUM_SWITCHES) return false;

  int switchByte = switchNum/8;
  int switchBit = switchNum%8;
  if ( ((SwitchesNow[switchByte])>>switchBit) & 0x01 ) return true;
  else return false;
}


byte RPU_GetDipSwitches(byte index) {
#ifdef RPU_OS_USE_DIP_SWITCHES
  if (index>3) return 0x00;
  return DipSwitches[index];
#else
  return 0x00 & index;
#endif
}


void RPU_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches, PlayfieldAndCabinetSwitch *s_gameSwitchArray) {
  NumGameSwitches = s_numSwitches;
  NumGamePrioritySwitches = s_numPrioritySwitches;
  GameSwitches = s_gameSwitchArray;
}


#if (RPU_MPU_ARCHITECTURE<10)
void RPU_ClearUpDownSwitchState() {
  return;
}

boolean RPU_GetUpDownSwitchState() {
  return true;
}
#else
void RPU_ClearUpDownSwitchState() {
  UpDownSwitch = false;
}

boolean RPU_GetUpDownSwitchState() {
  return UpDownSwitch;
}
#endif



/******************************************************
 *   Solenoid Handling Functions
 */

int SpaceLeftOnSolenoidStack() {
  if (SolenoidStackFirst>=SOLENOID_STACK_SIZE || SolenoidStackLast>=SOLENOID_STACK_SIZE) return 0;
  if (SolenoidStackLast>=SolenoidStackFirst) return ((SOLENOID_STACK_SIZE-1) - (SolenoidStackLast-SolenoidStackFirst));
  return (SolenoidStackFirst - SolenoidStackLast) - 1;
}


void RPU_PushToSolenoidStack(byte solenoidNumber, byte numPushes, boolean disableOverride) {
  if (solenoidNumber>14) return;

  // if the solenoid stack is disabled and this isn't an override push, then return
  if (!disableOverride && !SolenoidStackEnabled) return;

  // If the solenoid stack last index is out of range, then it's an error - return
  if (SpaceLeftOnSolenoidStack()==0) return;

  for (int count=0; count<numPushes; count++) {
    SolenoidStack[SolenoidStackLast] = solenoidNumber;
    
    SolenoidStackLast += 1;
    if (SolenoidStackLast==SOLENOID_STACK_SIZE) {
      // If the end index is off the end, then wrap
      SolenoidStackLast = 0;
    }
    // If the stack is now full, return
    if (SpaceLeftOnSolenoidStack()==0) return;
  }
}

void PushToFrontOfSolenoidStack(byte solenoidNumber, byte numPushes) {
  // If the stack is full, return
  if (SpaceLeftOnSolenoidStack()==0  || !SolenoidStackEnabled) return;

  for (int count=0; count<numPushes; count++) {
    if (SolenoidStackFirst==0) SolenoidStackFirst = SOLENOID_STACK_SIZE-1;
    else SolenoidStackFirst -= 1;
    SolenoidStack[SolenoidStackFirst] = solenoidNumber;
    if (SpaceLeftOnSolenoidStack()==0) return;
  }
  
}

byte PullFirstFromSolenoidStack() {
  // If first and last are equal, there's nothing on the stack
  if (SolenoidStackFirst==SolenoidStackLast) return SOLENOID_STACK_EMPTY;
  
  byte retVal = SolenoidStack[SolenoidStackFirst];

  SolenoidStackFirst += 1;
  if (SolenoidStackFirst>=SOLENOID_STACK_SIZE) SolenoidStackFirst = 0;

  return retVal;
}

boolean RPU_PushToTimedSolenoidStack(byte solenoidNumber, byte numPushes, unsigned long whenToFire, boolean disableOverride) {
  for (int count=0; count<TIMED_SOLENOID_STACK_SIZE; count++) {
    if (!TimedSolenoidStack[count].inUse) {
      TimedSolenoidStack[count].inUse = true;
      TimedSolenoidStack[count].pushTime = whenToFire;
      TimedSolenoidStack[count].disableOverride = disableOverride;
      TimedSolenoidStack[count].solenoidNumber = solenoidNumber;
      TimedSolenoidStack[count].numPushes = numPushes;
      return true;
    }
  }
  return false;
}

void RPU_UpdateTimedSolenoidStack(unsigned long curTime) {
  for (int count=0; count<TIMED_SOLENOID_STACK_SIZE; count++) {
    if (TimedSolenoidStack[count].inUse && TimedSolenoidStack[count].pushTime<curTime) {
      RPU_PushToSolenoidStack(TimedSolenoidStack[count].solenoidNumber, TimedSolenoidStack[count].numPushes, TimedSolenoidStack[count].disableOverride);
      TimedSolenoidStack[count].inUse = false;
    }
  }
}

#if (RPU_MPU_ARCHITECTURE<10)

void RPU_SetCoinLockout(boolean lockoutOff, byte solbit) {
  if (!lockoutOff) {
    CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
  } else {
    CurrentSolenoidByte = CurrentSolenoidByte | solbit;
  }
  RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}


void RPU_SetDisableFlippers(boolean disableFlippers, byte solbit) {
  if (disableFlippers) {
    CurrentSolenoidByte = CurrentSolenoidByte | solbit;
  } else {
    CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
  }
  
  RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}


void RPU_SetContinuousSolenoidBit(boolean bitOn, byte solbit) {
  if (bitOn) {
    CurrentSolenoidByte = CurrentSolenoidByte | solbit;
  } else {
    CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
  }
  RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}



boolean RPU_FireContinuousSolenoid(byte solBit, byte numCyclesToFire) {
  if (NumCyclesBeforeRevertingSolenoidByte) return false;

  NumCyclesBeforeRevertingSolenoidByte = numCyclesToFire;

  RevertSolenoidBit = solBit;
  RPU_SetContinuousSolenoidBit(false, solBit);
  return true;
}


byte RPU_ReadContinuousSolenoids() {
  return RPU_DataRead(ADDRESS_U11_B);
}


void RPU_DisableSolenoidStack() {
  SolenoidStackEnabled = false;
}


void RPU_EnableSolenoidStack() {
  SolenoidStackEnabled = true;
}

#elif (RPU_MPU_ARCHITECTURE>=10)
void RPU_SetDisableFlippers(boolean disableFlippers, byte solbit) {
  (void)solbit;
  if (disableFlippers) RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x34);
  else RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x3C);
}


void RPU_SetContinuousSolenoid(boolean solOn, byte solNum) {
  unsigned short oldCont = ContinuousSolenoidBits;
  if (solOn) ContinuousSolenoidBits |= (1<<solNum);
  else ContinuousSolenoidBits &= ~(1<<solNum);

  if (oldCont!=ContinuousSolenoidBits) {
    byte origPortA = RPU_DataRead(PIA_SOLENOID_PORT_A);
    byte origPortB = RPU_DataRead(PIA_SOLENOID_PORT_B);
    if (origPortA!=(ContinuousSolenoidBits&0xFF)) RPU_DataWrite(PIA_SOLENOID_PORT_A, (ContinuousSolenoidBits&0xFF));
    if (origPortB!=(ContinuousSolenoidBits/256)) RPU_DataWrite(PIA_SOLENOID_PORT_B, (ContinuousSolenoidBits/256));
  }
}

byte RPU_ReadContinuousSolenoids() {
  return ContinuousSolenoidBits;
}


void RPU_SetCoinLockout(boolean lockoutOn, byte solNum) {
  RPU_SetContinuousSolenoid(lockoutOn, solNum);  
}

void RPU_DisableSolenoidStack() {
  SolenoidStackEnabled = false;
  RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x34);
}


void RPU_EnableSolenoidStack() {
  SolenoidStackEnabled = true;
  RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x3C);
}


#endif 





/******************************************************
 *   Display Handling Functions
 */
#if (RPU_MPU_ARCHITECTURE<15)
byte RPU_SetDisplay(int displayNumber, unsigned long value, boolean blankByMagnitude, byte minDigits) {
  if (displayNumber<0 || displayNumber>4) return 0;

  byte blank = 0x00;

  for (int count=0; count<RPU_OS_NUM_DIGITS; count++) {
    blank = blank * 2;
    if (value!=0 || count<minDigits) blank |= 1;
    DisplayDigits[displayNumber][(RPU_OS_NUM_DIGITS-1)-count] = value%10;
    value /= 10;    
  }

  if (blankByMagnitude) DisplayDigitEnable[displayNumber] = blank;

  return blank;
}
#endif


#if (RPU_MPU_ARCHITECTURE<10)
void RPU_SetDisplayCredits(int value, boolean displayOn, boolean showBothDigits) {
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
  DisplayDigits[4][2] = (value%100) / 10;
  DisplayDigits[4][3] = (value%10);
#else
  DisplayDigits[4][1] = (value%100) / 10;
  DisplayDigits[4][2] = (value%10);
#endif 
  byte enableMask = DisplayDigitEnable[4] & RPU_OS_MASK_SHIFT_1;

  if (displayOn) {
    if (value>9 || showBothDigits) enableMask |= RPU_OS_MASK_SHIFT_2; 
    else enableMask |= 0x04;
  }

  DisplayDigitEnable[4] = enableMask;
}

void RPU_SetDisplayBallInPlay(int value, boolean displayOn, boolean showBothDigits) {
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
  DisplayDigits[4][5] = (value%100) / 10;
  DisplayDigits[4][6] = (value%10); 
#else
  DisplayDigits[4][4] = (value%100) / 10;  
  DisplayDigits[4][5] = (value%10); 
#endif
  byte enableMask = DisplayDigitEnable[4] & RPU_OS_MASK_SHIFT_2;

  if (displayOn) {
    if (value>9 || showBothDigits) enableMask |= RPU_OS_MASK_SHIFT_1;
    else enableMask |= 0x20;
  }

  DisplayDigitEnable[4] = enableMask;
}

#elif (RPU_MPU_ARCHITECTURE<15)

void RPU_SetDisplayCredits(int value, boolean displayOn, boolean showBothDigits) {
  byte blank = 0x02;
  value = value % 100;
  if (value>=10) {
    DisplayCreditDigits[0] = value/10;
    blank |= 1;
  } else {
    DisplayCreditDigits[0] = 0;
    if (showBothDigits) blank |= 1;
  }
  DisplayCreditDigits[1] = value%10;
  if (displayOn) DisplayCreditDigitEnable = blank;
  else DisplayCreditDigitEnable = 0;
}

void RPU_SetDisplayBallInPlay(int value, boolean displayOn, boolean showBothDigits) {
  byte blank = 0x02;
  value = value % 100;
  if (value>=10) {
    DisplayBIPDigits[0] = value/10;
    blank |= 1;
  } else {
    DisplayBIPDigits[0] = 0;
    if (showBothDigits) blank |= 1;
  }
  DisplayBIPDigits[1] = value%10;
  if (displayOn) DisplayBIPDigitEnable = blank;
  else DisplayBIPDigitEnable = 0;  
}

#endif

void RPU_CycleAllDisplays(unsigned long curTime, byte digitNum) {
  int displayDigit = (curTime/250)%10;
  unsigned long value;
#if (RPU_OS_NUM_DIGITS==7)
  value = displayDigit*1111111;
#else  
  value = displayDigit*111111;
#endif

  byte displayNumToShow = 0;
  byte displayBlank = RPU_OS_ALL_DIGITS_MASK;

  if (digitNum!=0) {
#if (RPU_OS_NUM_DIGITS==7)
    displayNumToShow = (digitNum-1)/7;
    displayBlank = (0x40)>>((digitNum-1)%7);

#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
    if (displayNumToShow==4) {
      displayBlank = (0x20)>>((digitNum-1)%6);
    }
#endif
    
#else    
    displayNumToShow = (digitNum-1)/6;
    displayBlank = (0x20)>>((digitNum-1)%6);
#endif
  }

  for (int count=0; count<5; count++) {
    if (digitNum) {
      RPU_SetDisplay(count, value);
      if (count==displayNumToShow) RPU_SetDisplayBlank(count, displayBlank);
      else RPU_SetDisplayBlank(count, 0);
    } else {
      RPU_SetDisplay(count, value, false);
    }
  }
}

void RPU_SetDisplayMatch(int value, boolean displayOn, boolean showBothDigits) {
  RPU_SetDisplayBallInPlay(value, displayOn, showBothDigits);
}


// This is confusing -
// Digit mask is like this
//   bit=   b7 b6 b5 b4 b3 b2 b1 b0
//   digit=  x  x  6  5  4  3  2  1
//   (with digit 6 being the least-significant, 1's digit
//  
// so, looking at it from left to right on the display
//   digit=  1  2  3  4  5  6
//   bit=   b0 b1 b2 b3 b4 b5
void RPU_SetDisplayBlank(int displayNumber, byte bitMask) {
  if (displayNumber<0 || displayNumber>4) return;
    
  DisplayDigitEnable[displayNumber] = bitMask;
}

byte RPU_GetDisplayBlank(int displayNumber) {
  if (displayNumber<0 || displayNumber>4) return 0;
  return DisplayDigitEnable[displayNumber];
}

#if defined(RPU_OS_ADJUSTABLE_DISPLAY_INTERRUPT)
void RPU_SetDisplayRefreshConstant(int intervalConstant) {
  cli();
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for selected increment
  OCR1A = intervalConstant;
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();
}
#endif


void RPU_SetDisplayFlash(int displayNumber, unsigned long value, unsigned long curTime, int period, byte minDigits) {
  // A period of zero toggles display every other time
  if (period) {
    if ((curTime/period)%2) {
      RPU_SetDisplay(displayNumber, value, true, minDigits);
    } else {
      RPU_SetDisplayBlank(displayNumber, 0);
    }
  }
  
}

void RPU_SetDisplayFlashCredits(unsigned long curTime, int period) {
  if (period) {
    if ((curTime/period)%2) {
      DisplayDigitEnable[4] |= 0x06;
    } else {
      DisplayDigitEnable[4] &= 0x39;
    }
  }
}


#if (RPU_MPU_ARCHITECTURE==15)
byte RPU_SetDisplayText(int displayNumber, char *text, boolean blankByLength) {
  if (displayNumber>1 || displayNumber<0) return 0;
  byte stringLength = 0xff;
  boolean writeSpace = false;
  byte blank = 0;
  byte placeMask = 0x01;

  for (stringLength=0; stringLength<RPU_OS_NUM_DIGITS; stringLength++) {
    if (text[stringLength]==0) writeSpace = true;
    if (!writeSpace) DisplayText[displayNumber][stringLength] = (byte)text[stringLength]-0x20;
    else DisplayText[displayNumber][stringLength] = 0;

    if (DisplayText[displayNumber][stringLength]) blank |= placeMask;
    placeMask *= 2;
  }

  if (blankByLength) DisplayDigitEnable[displayNumber] = blank;

  return stringLength;
}

// Architectures with alpha store numbers as 7-seg
byte RPU_SetDisplay(int displayNumber, unsigned long value, boolean blankByMagnitude, byte minDigits) {
  if (displayNumber<0 || displayNumber>3) return 0;

  byte blank = 0x00;

  for (int count=0; count<RPU_OS_NUM_DIGITS; count++) {
    blank = blank * 2;
    if (value!=0 || count<minDigits) {
      blank |= 1;
      if (displayNumber/2) DisplayDigits[displayNumber][(RPU_OS_NUM_DIGITS-1)-count] = SevenSegmentNumbers[value%10];
      else DisplayText[displayNumber][(RPU_OS_NUM_DIGITS-1)-count] = (value%10)+16;
    } else {
      if (displayNumber/2) DisplayDigits[displayNumber][(RPU_OS_NUM_DIGITS-1)-count] = 0;
      else DisplayText[displayNumber][(RPU_OS_NUM_DIGITS-1)-count] = 0;
    }
    value /= 10;    
  }
  
  if (blankByMagnitude) DisplayDigitEnable[displayNumber] = blank;
  
  return blank;
}


void RPU_SetDisplayCredits(int value, boolean displayOn, boolean showBothDigits) {
  byte blank = 0x02;
  value = value % 100;
  if (value>=10) {
    DisplayCreditDigits[0] = SevenSegmentNumbers[value/10];
    blank |= 1;
  } else {
    DisplayCreditDigits[0] = SevenSegmentNumbers[0];
    if (showBothDigits) blank |= 1;
  }
  DisplayCreditDigits[1] = SevenSegmentNumbers[value%10];
  if (displayOn) DisplayCreditDigitEnable = blank;
  else DisplayCreditDigitEnable = 0;
}

void RPU_SetDisplayBallInPlay(int value, boolean displayOn, boolean showBothDigits) {
  byte blank = 0x02;
  value = value % 100;
  if (value>=10) {
    DisplayBIPDigits[0] = SevenSegmentNumbers[value/10];
    blank |= 1;
  } else {
    DisplayBIPDigits[0] = SevenSegmentNumbers[0];
    if (showBothDigits) blank |= 1;
  }
  DisplayBIPDigits[1] = SevenSegmentNumbers[value%10];
  if (displayOn) DisplayBIPDigitEnable = blank;
  else DisplayBIPDigitEnable = 0;  
}

#endif


/******************************************************
 *   Lamp Handling Functions
 */

void RPU_SetDimDivisor(byte level, byte divisor) {
  if (level==1) DimDivisor1 = divisor;
  if (level==2) DimDivisor2 = divisor;
}

void RPU_SetLampState(int lampNum, byte s_lampState, byte s_lampDim, int s_lampFlashPeriod) {
  if (lampNum>=RPU_MAX_LAMPS || lampNum<0) return;
  
  if (s_lampState) {
    int adjustedLampFlash = s_lampFlashPeriod/50;
    
    if (s_lampFlashPeriod!=0 && adjustedLampFlash==0) adjustedLampFlash = 1;
    if (adjustedLampFlash>250) adjustedLampFlash = 250;
    
    // Only turn on the lamp if there's no flash, because if there's a flash
    // then the lamp will be turned on by the ApplyFlashToLamps function
    if (s_lampFlashPeriod==0) LampStates[lampNum/8] &= ~(0x01<<(lampNum%8));
    LampFlashPeriod[lampNum] = adjustedLampFlash;
  } else {
    LampStates[lampNum/8] |= (0x01<<(lampNum%8));
    LampFlashPeriod[lampNum] = 0;
  }

  if (s_lampDim & 0x01) {    
    LampDim1[lampNum/8] |= (0x01<<(lampNum%8));
  } else {
    LampDim1[lampNum/8] &= ~(0x01<<(lampNum%8));
  }

  if (s_lampDim & 0x02) {    
    LampDim2[lampNum/8] |= (0x01<<(lampNum%8));
  } else {
    LampDim2[lampNum/8] &= ~(0x01<<(lampNum%8));
  }

}

byte RPU_ReadLampState(int lampNum) {
  if (lampNum>=RPU_MAX_LAMPS || lampNum<0) return 0x00;
  byte lampStateByte = LampStates[lampNum/8];
  return (lampStateByte & (0x01<<(lampNum%8))) ? 0 : 1;
}

byte RPU_ReadLampDim(int lampNum) {
  if (lampNum>=RPU_MAX_LAMPS || lampNum<0) return 0x00;
  byte lampDim = 0;
  byte lampDimByte = LampDim1[lampNum/8];
  if (lampDimByte & (0x01<<(lampNum%8))) lampDim |= 1;

  lampDimByte = LampDim2[lampNum/8];
  if (lampDimByte & (0x01<<(lampNum%8))) lampDim |= 2;

  return lampDim;
}

int RPU_ReadLampFlash(int lampNum) {
  if (lampNum>=RPU_MAX_LAMPS || lampNum<0) return 0;

  return LampFlashPeriod[lampNum]*50;
}

void RPU_ApplyFlashToLamps(unsigned long curTime) {
  int curLampByte = 0;
  byte curLampBit = 0;
  int curLampNum = 0;

  for (curLampByte=0; curLampByte<RPU_NUM_LAMP_BANKS; curLampByte++) {
    curLampBit = 0x01;
    for (byte curBit=0; curBit<8; curBit++) {
      if ( LampFlashPeriod[curLampNum]!=0 ) {
        unsigned long adjustedLampFlash = (unsigned long)LampFlashPeriod[curLampNum] * (unsigned long)50;
        if ((curTime/adjustedLampFlash)%2) {
          LampStates[curLampByte] &= ~(curLampBit);
        } else {
          LampStates[curLampByte] |= (curLampBit);
        }
      }

      curLampBit *= 2;
      curLampNum += 1;
    }
  }
}

void RPU_FlashAllLamps(unsigned long curTime) {
  for (int count=0; count<RPU_MAX_LAMPS; count++) {
    RPU_SetLampState(count, 1, 0, 500);  
  }

  RPU_ApplyFlashToLamps(curTime);
}

void RPU_TurnOffAllLamps() {
  for (int count=0; count<RPU_MAX_LAMPS; count++) {
    RPU_SetLampState(count, 0, 0, 0);  
  }
}



/******************************************************
 *   Helper Functions
 */

void RPU_ClearVariables() {
  // Reset solenoid stack
  SolenoidStackFirst = 0;
  SolenoidStackLast = 0;

  // Reset switch stack
  SwitchStackFirst = 0;
  SwitchStackLast = 0;

#if (RPU_MPU_ARCHITECTURE > 9) 
  // Reset sound stack
  SoundStackFirst = 0;
  SoundStackLast = 0;
#endif

  CurrentDisplayDigit = 0; 

  // Set default values for the displays
  for (int displayCount=0; displayCount<5; displayCount++) {
    for (int digitCount=0; digitCount<RPU_OS_NUM_DIGITS; digitCount++) {
      DisplayDigits[displayCount][digitCount] = 0;
    }
    DisplayDigitEnable[displayCount] = 0x00;
  }

  // Turn off all lamp states
  for (int lampBankCounter=0; lampBankCounter<RPU_NUM_LAMP_BANKS; lampBankCounter++) {
    LampStates[lampBankCounter] = 0xFF;
    LampDim1[lampBankCounter] = 0x00;
    LampDim2[lampBankCounter] = 0x00;
  }

  for (int lampFlashCount=0; lampFlashCount<RPU_MAX_LAMPS; lampFlashCount++) {
    LampFlashPeriod[lampFlashCount] = 0;
  }

  // Reset all the switch values 
  // (set them as closed so that if they're stuck they don't register as new events)
  byte switchCount;
  for (switchCount=0; switchCount<NUM_SWITCH_BYTES; switchCount++) {
    SwitchesMinus2[switchCount] = 0xFF;
    SwitchesMinus1[switchCount] = 0xFF;
    SwitchesNow[switchCount] = 0xFF;
  }

  for (byte count=0; count<TIMED_SOLENOID_STACK_SIZE; count++) {
    TimedSolenoidStack[count].inUse = 0;
    TimedSolenoidStack[count].pushTime = 0;
    TimedSolenoidStack[count].solenoidNumber = 0;
    TimedSolenoidStack[count].numPushes = 0;
    TimedSolenoidStack[count].disableOverride = 0;
  }

#if (RPU_MPU_ARCHITECTURE > 9) 
  for (byte count=0; count<TIMED_SOUND_STACK_SIZE; count++) {
    TimedSoundStack[count].inUse = 0;
    TimedSoundStack[count].pushTime = 0;
    TimedSoundStack[count].soundNumber = 0;
    TimedSoundStack[count].numPushes = 0;
  }  
#endif
  
}




/******************************************************
 *   Sound Handling Functions
 */
 
#ifdef RPU_OS_USE_S_AND_T

void RPU_PlaySoundSAndT(byte soundByte) {

  byte oldSolenoidControlByte, soundLowerNibble, soundUpperNibble;

  // mask further zero-crossing interrupts during this 
  noInterrupts();

  // Get the current value of U11:PortB - current solenoids
  oldSolenoidControlByte = RPU_DataRead(ADDRESS_U11_B);
  soundLowerNibble = (oldSolenoidControlByte&0xF0) | (soundByte&0x0F); 
  soundUpperNibble = (oldSolenoidControlByte&0xF0) | (soundByte/16); 
    
  // Put 1s on momentary solenoid lines
  RPU_DataWrite(ADDRESS_U11_B, oldSolenoidControlByte | 0x0F);

  // Put sound latch low
  RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);

  // Let the strobe stay low for a moment
  delayMicroseconds(32);

  // Put sound latch high
  RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x3C);
  
  // put the new byte on U11:PortB (the lower nibble is currently loaded)
  RPU_DataWrite(ADDRESS_U11_B, soundLowerNibble);
        
  // wait 138 microseconds
  delayMicroseconds(138);

  // put the new byte on U11:PortB (the uppper nibble is currently loaded)
  RPU_DataWrite(ADDRESS_U11_B, soundUpperNibble);

  // wait 76 microseconds
  delayMicroseconds(145);

  // Restore the original solenoid byte
  RPU_DataWrite(ADDRESS_U11_B, oldSolenoidControlByte);

  // Put sound latch low
  RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);

  interrupts();
}
#endif

// With hardware rev 1, this function relies on D13 being connected to A5 because it writes to address 0xA0
// A0  - A0   0
// A1  - A1   0   
// A2  - n/c  0
// A3  - A2   0
// A4  - A3   0
// A5  - D13  1
// A6  - n/c  0
// A7  - A4   1
// A8  - n/c  0
// A9  - GND  0
// A10 - n/c  0
// A11 - n/c  0
// A12 - GND  0
// A13 - n/c  0
#ifdef RPU_OS_USE_SB100
void RPU_PlaySB100(byte soundByte) {

#if (RPU_OS_HARDWARE_REV==1)
  PORTB = PORTB | 0x20;
#endif 

  RPU_DataWrite(ADDRESS_SB100, soundByte);

#if (RPU_OS_HARDWARE_REV==1)
  PORTB = PORTB & 0xDF;
#endif 
  
}

#if (RPU_OS_HARDWARE_REV==2)
void RPU_PlaySB100Chime(byte soundByte) {

  RPU_DataWrite(ADDRESS_SB100_CHIMES, soundByte);

}
#endif 
#endif


#ifdef RPU_OS_USE_DASH51
void RPU_PlaySoundDash51(byte soundByte) {

  // This device has 32 possible sounds, but they're mapped to 
  // 0 - 15 and then 128 - 143 on the original card, with bits b4, b5, and b6 reserved
  // for timing controls.
  // For ease of use, I've mapped the sounds from 0-31
  
  byte oldSolenoidControlByte, soundLowerNibble, displayWithSoundBit4, oldDisplayByte;

  // mask further zero-crossing interrupts during this 
  noInterrupts();

  // Get the current value of U11:PortB - current solenoids
  oldSolenoidControlByte = RPU_DataRead(ADDRESS_U11_B);
  oldDisplayByte = RPU_DataRead(ADDRESS_U11_A);
  soundLowerNibble = (oldSolenoidControlByte&0xF0) | (soundByte&0x0F); 
  displayWithSoundBit4 = oldDisplayByte;
  if (soundByte & 0x10) displayWithSoundBit4 |= 0x02;
  else displayWithSoundBit4 &= 0xFD;
    
  // Put 1s on momentary solenoid lines
  RPU_DataWrite(ADDRESS_U11_B, oldSolenoidControlByte | 0x0F);

  // Put sound latch low
  RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);

  // Let the strobe stay low for a moment
  delayMicroseconds(68);

  // put bit 4 on Display Enable 7
  RPU_DataWrite(ADDRESS_U11_A, displayWithSoundBit4);

  // Put sound latch high
  RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x3C);
  
  // put the new byte on U11:PortB (the lower nibble is currently loaded)
  RPU_DataWrite(ADDRESS_U11_B, soundLowerNibble);
        
  // wait 180 microseconds
  delayMicroseconds(180);

  // Restore the original solenoid byte
  RPU_DataWrite(ADDRESS_U11_B, oldSolenoidControlByte);

  // Restore the original display byte
  RPU_DataWrite(ADDRESS_U11_A, oldDisplayByte);

  // Put sound latch low
  RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);

  interrupts();
}

#endif

#if (RPU_OS_HARDWARE_REV>=2 && defined(RPU_OS_USE_SB300))

void RPU_PlaySB300SquareWave(byte soundRegister, byte soundByte) {
  RPU_DataWrite(ADDRESS_SB300_SQUARE_WAVES+soundRegister, soundByte);
}

void RPU_PlaySB300Analog(byte soundRegister, byte soundByte) {
  RPU_DataWrite(ADDRESS_SB300_ANALOG+soundRegister, soundByte);
}

#endif 


#if defined(RPU_OS_USE_WTYPE_1_SOUND) || defined(RPU_OS_USE_WTYPE_2_SOUND)
#if defined(RPU_OS_USE_WTYPE_2_SOUND) 
unsigned short SoundLowerLimit = 0x0000;
unsigned short SoundUpperLimit = 0x0080;
#else 
unsigned short SoundLowerLimit = 0x0100;
unsigned short SoundUpperLimit = 0x1F00;
#endif


void RPU_SetSoundValueLimits(unsigned short lowerLimit, unsigned short upperLimit) {
  SoundLowerLimit = lowerLimit;
  SoundUpperLimit = upperLimit;
}
 
int SpaceLeftOnSoundStack() {
  if (SoundStackFirst>=SOUND_STACK_SIZE || SoundStackLast>=SOUND_STACK_SIZE) return 0;
  if (SoundStackLast>=SoundStackFirst) return ((SOUND_STACK_SIZE-1) - (SoundStackLast-SoundStackFirst));
  return (SoundStackFirst - SoundStackLast) - 1;
}

void RPU_PushToSoundStack(unsigned short soundNumber, byte numPushes) {  
  // If the solenoid stack last index is out of range, then it's an error - return  
  if (SpaceLeftOnSoundStack()==0) return;
  if (soundNumber<SoundLowerLimit || soundNumber>SoundUpperLimit) return;

  for (int count=0; count<numPushes; count++) {
    SoundStack[SoundStackLast] = soundNumber;
    
    SoundStackLast += 1;
    if (SoundStackLast==SOUND_STACK_SIZE) {
      // If the end index is off the end, then wrap
      SoundStackLast = 0;
    }
    // If the stack is now full, return
    if (SpaceLeftOnSoundStack()==0) return;
  }

}


unsigned short PullFirstFromSoundStack() {
  // If first and last are equal, there's nothing on the stack
  if (SoundStackFirst==SoundStackLast) {
    return SOUND_STACK_EMPTY;
  }
  
  unsigned short retVal = SoundStack[SoundStackFirst];

  SoundStackFirst += 1;
  if (SoundStackFirst>=SOUND_STACK_SIZE) SoundStackFirst = 0;

  return retVal;
}


boolean RPU_PushToTimedSoundStack(unsigned short soundNumber, byte numPushes, unsigned long whenToPlay) {
  for (int count=0; count<TIMED_SOUND_STACK_SIZE; count++) {
    if (!TimedSoundStack[count].inUse) {
      TimedSoundStack[count].inUse = true;
      TimedSoundStack[count].pushTime = whenToPlay;
      TimedSoundStack[count].soundNumber = soundNumber;
      TimedSoundStack[count].numPushes = numPushes;
      return true;
    }
  }
  return false;
}


void RPU_UpdateTimedSoundStack(unsigned long curTime) { 
  for (int count=0; count<TIMED_SOUND_STACK_SIZE; count++) {
    if (TimedSoundStack[count].inUse && TimedSoundStack[count].pushTime<curTime) {
      RPU_PushToSoundStack(TimedSoundStack[count].soundNumber, TimedSoundStack[count].numPushes);
      TimedSoundStack[count].inUse = false;
    }
  }
}
#endif

#ifdef RPU_OS_USE_WTYPE_11_SOUND
void RPU_PlayW11Sound(byte soundNum) {
  RPU_DataWrite(PIA_SOUND_11_PORT_A, soundNum);
  // Strobe CA2
  RPU_DataWrite(PIA_SOUND_11_CONTROL_A, 0x34);
  RPU_DataWrite(PIA_SOUND_11_CONTROL_A, 0x3C);
}

void RPU_PlayW11Music(byte songNum) {
  RPU_DataWrite(PIA_WIDGET_PORT_B, songNum);
  // Strobe CA2
  RPU_DataWrite(PIA_WIDGET_CONTROL_B, 0x34);
  RPU_DataWrite(PIA_WIDGET_CONTROL_B, 0x3C);
}
#endif




/******************************************************
 *   EEPROM Helper Functions
 */

void RPU_WriteByteToEEProm(unsigned short startByte, byte value) {
  EEPROM.write(startByte, value);
}

byte RPU_ReadByteFromEEProm(unsigned short startByte) {
  byte value = EEPROM.read(startByte);

  // If this value is unset, set it
  if (value==0xFF) {
    value = 0;
    RPU_WriteByteToEEProm(startByte, value);
  }
  return value;
}


unsigned long RPU_ReadULFromEEProm(unsigned short startByte, unsigned long defaultValue) {
  unsigned long value;

  value = (((unsigned long)EEPROM.read(startByte+3))<<24) | 
          ((unsigned long)(EEPROM.read(startByte+2))<<16) | 
          ((unsigned long)(EEPROM.read(startByte+1))<<8) | 
          ((unsigned long)(EEPROM.read(startByte)));

  if (value==0xFFFFFFFF) {
    value = defaultValue; 
    RPU_WriteULToEEProm(startByte, value);
  }
  return value;
}


void RPU_WriteULToEEProm(unsigned short startByte, unsigned long value) {
  EEPROM.write(startByte+3, (byte)(value>>24));
  EEPROM.write(startByte+2, (byte)((value>>16) & 0x000000FF));
  EEPROM.write(startByte+1, (byte)((value>>8) & 0x000000FF));
  EEPROM.write(startByte, (byte)(value & 0x000000FF));
}




/******************************************************
 *   Initialization and ISR Functions
 */
#if (RPU_OS_HARDWARE_REV==102)
boolean CheckForMPUClock() {
  pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
  digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 1);
  pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
  digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
  pinMode(RPU_PHI2_PIN, INPUT_PULLUP);
  
  unsigned long startTime = millis();
  int sawClockLow = 0;
  int sawClockHigh = 0;  
  while (millis()<(startTime + 10)) {
    if (PING & 0x04) sawClockHigh += 1;
    else sawClockLow += 1;
  }
  
  if (sawClockLow>25 && sawClockHigh>25) {
    return true;
  }

  // At this point, since we didn't see the mpu clock, we
  // can assume that the clock is generated by the CPU,
  // but we can check if we want
  digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 0);

  sawClockLow = 0;
  sawClockHigh = 0;
  startTime = millis();
  byte lastState = 0;
  for (int count=0; count<1000; count++) {
    if (PING & 0x04 && !lastState) {
      sawClockHigh += 1;
      lastState = 1;
    } else if (lastState) {
      sawClockLow += 1;
      lastState = 0;
    }
  }
  
  digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
  
  return false;
}
#endif
 
#if (RPU_MPU_ARCHITECTURE<10)

volatile int numberOfU10Interrupts = 0;
volatile int numberOfU11Interrupts = 0;
volatile byte InsideZeroCrossingInterrupt = 0;

ISR(TIMER1_COMPA_vect) {    //This is the interrupt request
  // Backup U10A
  byte backupU10A = RPU_DataRead(ADDRESS_U10_A);
  
  // Disable lamp decoders & strobe latch
  RPU_DataWrite(ADDRESS_U10_A, 0xFF);
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);
#ifdef RPU_OS_USE_AUX_LAMPS
  // Also park the aux lamp board 
  RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
  RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);    
#endif

  // Blank Displays
  RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL) & 0xF7);
  // Set all 5 display latch strobes high
  RPU_DataWrite(ADDRESS_U11_A, (RPU_DataRead(ADDRESS_U11_A)/* & 0x03*/) | 0x01);
  RPU_DataWrite(ADDRESS_U10_A, 0x0F);

  byte displayStrobeMask = 0x01;
  byte displayDigitsMask;
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS          
  displayDigitsMask = (0x02<<CurrentDisplayDigit);
#else
  displayDigitsMask = RPU_DataRead(ADDRESS_U11_A) & 0x02;
  displayDigitsMask |= (0x04<<CurrentDisplayDigit);
#endif          
      
  // Write current display digits to 5 displays
  for (int displayCount=0; displayCount<5; displayCount++) {

    // The BCD for this digit is in b4-b7, and the display latch strobes are in b0-b3 (and U11A:b0)
    byte displayDataByte = ((DisplayDigits[displayCount][CurrentDisplayDigit])<<4) | 0x0F;
    byte displayEnable = ((DisplayDigitEnable[displayCount])>>CurrentDisplayDigit)&0x01;

    // if this digit shouldn't be displayed, then set data lines to 0xFX so digit will be blank
    if (!displayEnable) displayDataByte = 0xFF;

    // Calculate which bit needs to be dropped
    if (displayCount<4) {
      displayDataByte &= ~(displayStrobeMask);
    }

    // Write out the digit & strobe (if it's 0-3)
    // The current number to display is the upper nibble of displayDataByte, 
    // and the lower nibble is the strobe lines for the four score displays.
    // The strobe for the four score displays is high here because then the strobes
    // are NOR'd with U10:CA2 (which mutes the signals during other actions).
    // Only one strobe is low (from the above line. 
    RPU_DataWrite(ADDRESS_U10_A, displayDataByte);
    if (displayCount==4) {            
      // Strobe #5 latch on U11A:b0
      RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask & 0xFE);
    }

    // Right now the "Display Latch Strobe" is high

    // Put the latch strobe bits back high (low on the port)
    delayMicroseconds(16);
    if (displayCount<4) {
      displayDataByte |= 0x0F;
      // Need to delay a little to make sure the strobe is low (high on the port) for long enough
      RPU_DataWrite(ADDRESS_U10_A, displayDataByte);
    } else {
      RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask | 0x01);        
    }
    
    displayStrobeMask *= 2;
  }

  // While the data is being strobed, we need to enable the current digit
  RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask | 0x01);

  CurrentDisplayDigit = CurrentDisplayDigit + 1;
  if (CurrentDisplayDigit>=RPU_OS_NUM_DIGITS) {
    CurrentDisplayDigit = 0;
    DisplayOffCycle ^= true;
  }

  // Stop Blanking (current digits are all latched and ready)
  RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL) | 0x08);

  // Restore 10A from backup
  RPU_DataWrite(ADDRESS_U10_A, backupU10A);    

}

void InterruptService3() {
  byte u10AControl = RPU_DataRead(ADDRESS_U10_A_CONTROL);
  if (u10AControl & 0x80) {
    // self test switch
    if (RPU_DataRead(ADDRESS_U10_A_CONTROL) & 0x80) PushToSwitchStack(SW_SELF_TEST_SWITCH);
    RPU_DataRead(ADDRESS_U10_A);
  }

  // If we get a weird interupt from U11B, clear it
  byte u11BControl = RPU_DataRead(ADDRESS_U11_B_CONTROL);
  if (u11BControl & 0x80) {
    RPU_DataRead(ADDRESS_U11_B);    
  }

  byte u11AControl = RPU_DataRead(ADDRESS_U11_A_CONTROL);
  byte u10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);

  // If the interrupt bit on the display interrupt is on, do the display refresh
  if (u11AControl & 0x80) {
    RPU_DataRead(ADDRESS_U11_A);
    numberOfU11Interrupts+=1;
  }

  // If the IRQ bit of U10BControl is set, do the Zero-crossing interrupt handler
  if ((u10BControl & 0x80) && (InsideZeroCrossingInterrupt==0)) {
    InsideZeroCrossingInterrupt = InsideZeroCrossingInterrupt + 1;

    byte u10BControlLatest = RPU_DataRead(ADDRESS_U10_B_CONTROL);

    // Backup contents of U10A
    byte backup10A = RPU_DataRead(ADDRESS_U10_A);

    // Latch 0xFF separately without interrupt clear
    RPU_DataWrite(ADDRESS_U10_A, 0xFF);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);
    // Read U10B to clear interrupt
    RPU_DataRead(ADDRESS_U10_B);

    // Turn off U10BControl interrupts
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x30);

    // Copy old switch values
    byte switchCount;
    byte startingClosures;
    byte validClosures;
    for (switchCount=0; switchCount<NUM_SWITCH_BYTES; switchCount++) {
      SwitchesMinus2[switchCount] = SwitchesMinus1[switchCount];
      SwitchesMinus1[switchCount] = SwitchesNow[switchCount];

      // Enable switch strobe
#if defined(RPU_USE_EXTENDED_SWITCHES_ON_PB4) or defined(RPU_USE_EXTENDED_SWITCHES_ON_PB7)
      if (switchCount<NUM_SWITCH_BYTES_ON_U10_PORT_A) {
        RPU_DataWrite(ADDRESS_U10_A, 0x01<<switchCount);
      } else {
        RPU_SetContinuousSolenoidBit(true, ST5_CONTINUOUS_SOLENOID_BIT);
      }
#else       
      RPU_DataWrite(ADDRESS_U10_A, 0x01<<switchCount);
#endif        

      // Turn off U10:CB2 if it's on (because it strobes the last bank of dip switches
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x34);

      // Delay for switch capacitors to charge
      delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
      
      // Read the switches
      SwitchesNow[switchCount] = RPU_DataRead(ADDRESS_U10_B);

      //Unset the strobe
      RPU_DataWrite(ADDRESS_U10_A, 0x00);
#if defined(RPU_USE_EXTENDED_SWITCHES_ON_PB4) or defined(RPU_USE_EXTENDED_SWITCHES_ON_PB7)
      RPU_SetContinuousSolenoidBit(false, ST5_CONTINUOUS_SOLENOID_BIT);
#endif 

      // Some switches need to trigger immediate closures (bumpers & slings)
      startingClosures = (SwitchesNow[switchCount]) & (~SwitchesMinus1[switchCount]);
      boolean immediateSolenoidFired = false;
      // If one of the switches is starting to close (off, on)
      if (startingClosures) {
        // Loop on bits of switch byte
        for (byte bitCount=0; bitCount<8 && immediateSolenoidFired==false; bitCount++) {
          // If this switch bit is closed
          if (startingClosures&0x01) {
            byte startingSwitchNum = switchCount*8 + bitCount;
            // Loop on immediate switch data
            for (int immediateSwitchCount=0; immediateSwitchCount<NumGamePrioritySwitches && immediateSolenoidFired==false; immediateSwitchCount++) {
              // If this switch requires immediate action
              if (GameSwitches && startingSwitchNum==GameSwitches[immediateSwitchCount].switchNum) {
                // Start firing this solenoid (just one until the closure is validate
                PushToFrontOfSolenoidStack(GameSwitches[immediateSwitchCount].solenoid, 1);
                immediateSolenoidFired = true;
              }
            }
          }
          startingClosures = startingClosures>>1;
        }
      }

      immediateSolenoidFired = false;
      validClosures = (SwitchesNow[switchCount] & SwitchesMinus1[switchCount]) & ~SwitchesMinus2[switchCount];
      // If there is a valid switch closure (off, on, on)
      if (validClosures) {
        // Loop on bits of switch byte
        for (byte bitCount=0; bitCount<8; bitCount++) {
          // If this switch bit is closed
          if (validClosures&0x01) {
            byte validSwitchNum = switchCount*8 + bitCount;
            // Loop through all switches and see what's triggered
            for (int validSwitchCount=0; validSwitchCount<NumGameSwitches; validSwitchCount++) {

              // If we've found a valid closed switch
              if (GameSwitches && GameSwitches[validSwitchCount].switchNum==validSwitchNum) {

                // If we're supposed to trigger a solenoid, then do it
                if (GameSwitches[validSwitchCount].solenoid!=SOL_NONE) {
                  if (validSwitchCount<NumGamePrioritySwitches && immediateSolenoidFired==false) {
                    PushToFrontOfSolenoidStack(GameSwitches[validSwitchCount].solenoid, GameSwitches[validSwitchCount].solenoidHoldTime);
                  } else {
                    RPU_PushToSolenoidStack(GameSwitches[validSwitchCount].solenoid, GameSwitches[validSwitchCount].solenoidHoldTime);
                  }
                } // End if this is a real solenoid
              } // End if this is a switch in the switch table
            } // End loop on switches in switch table
            // Push this switch to the game rules stack
            PushToSwitchStack(validSwitchNum);
          }
          validClosures = validClosures>>1;
        }        
      }

      // There are no port reads or writes for the rest of the loop, 
      // so we can allow the display interrupt to fire
      interrupts();
      
      // Wait so total delay will allow lamp SCRs to get to the proper voltage
      delayMicroseconds(RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS);
      
      noInterrupts();
    }
    RPU_DataWrite(ADDRESS_U10_A, backup10A);

    if (NumCyclesBeforeRevertingSolenoidByte!=0) {
      NumCyclesBeforeRevertingSolenoidByte -= 1;
      if (NumCyclesBeforeRevertingSolenoidByte==0) {
        CurrentSolenoidByte |= RevertSolenoidBit;
        RevertSolenoidBit = 0x00;
      }
    }

#ifdef RPU_OS_USE_DASH32
    // mask out sound E line
    byte curDisplayDigitEnableByte = RPU_DataRead(ADDRESS_U11_A);
    RPU_DataWrite(ADDRESS_U11_A, curDisplayDigitEnableByte | 0x02);
#endif    

    // If we need to turn off momentary solenoids, do it first
    byte momentarySolenoidAtStart = PullFirstFromSolenoidStack();
    if (momentarySolenoidAtStart!=SOLENOID_STACK_EMPTY) {
      CurrentSolenoidByte = (CurrentSolenoidByte&0xF0) | momentarySolenoidAtStart;
      RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
#ifdef RPU_OS_USE_DASH32
      // Raise CB2 so we don't unset the solenoid we just set
      RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x3C);
      // Mask off sound lines
      RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte | SOL_NONE);
      // Put CB2 back low
      RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);
      // Put solenoids back again
      RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
#endif    
    } else {
      CurrentSolenoidByte = (CurrentSolenoidByte&0xF0) | SOL_NONE;
      RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
    }

#ifdef RPU_OS_USE_DASH32
    // put back U11 A without E line
    RPU_DataWrite(ADDRESS_U11_A, curDisplayDigitEnableByte);
#endif    

    for (int lampByteCount=0; lampByteCount<8; lampByteCount++) {
      for (byte nibbleCount=0; nibbleCount<2; nibbleCount++) {
        
        // We skip iteration number 16 because the last position is to park the lamps
        if (lampByteCount==(7) && nibbleCount) continue;
        
        byte lampData = 0xF0 + (lampByteCount*2) + nibbleCount;

        interrupts();
        RPU_DataWrite(ADDRESS_U10_A, 0xFF);
        noInterrupts();
      
        // Latch address & strobe
        RPU_DataWrite(ADDRESS_U10_A, lampData);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE      
        delayMicroseconds(2);
#endif      

        RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x38);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE      
        delayMicroseconds(2);
#endif      

        RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x30);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE      
        delayMicroseconds(2);
#endif      

        // Use the inhibit lines to set the actual data to the lamp SCRs 
        // (here, we don't care about the lower nibble because the address was already latched)
        byte nibbleOffset = (nibbleCount)?1:16;
        byte lampOutput = (LampStates[lampByteCount] * nibbleOffset);
        // Every other time through the cycle, we OR in the dim variable
        // in order to dim those lights
        if (numberOfU10Interrupts%DimDivisor1) lampOutput |= (LampDim1[lampByteCount] * nibbleOffset);
        if (numberOfU10Interrupts%DimDivisor2) lampOutput |= (LampDim2[lampByteCount] * nibbleOffset);

        RPU_DataWrite(ADDRESS_U10_A, lampOutput & 0xF0);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE      
        delayMicroseconds(2);
#endif      
      } // end loop on nibble
    } // end loop on lamp bytes


#ifdef RPU_OS_USE_AUX_LAMPS
    // Latch 0xFF separately without interrupt clear
    // to park 0xFF in main lamp board
    RPU_DataWrite(ADDRESS_U10_A, 0xFF);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

    for (int lampByteCount=8; lampByteCount<RPU_NUM_LAMP_BANKS; lampByteCount++) {
      for (byte nibbleCount=0; nibbleCount<2; nibbleCount++) {
        byte nibbleOffset = (nibbleCount)?1:16;
        byte lampOutput = (LampStates[lampByteCount] * nibbleOffset);
        // Every other time through the cycle, we OR in the dim variable
        // in order to dim those lights
        if (numberOfU10Interrupts%DimDivisor1) lampOutput |= (LampDim1[lampByteCount] * nibbleOffset);
        if (numberOfU10Interrupts%DimDivisor2) lampOutput |= (LampDim2[lampByteCount] * nibbleOffset);

        // The data will be in the upper nibble, but we need the bank count in the lower
        lampOutput &= 0xF0;
        lampOutput += ((lampByteCount-8)*2+nibbleOffset);

        interrupts();
        RPU_DataWrite(ADDRESS_U10_A, 0xFF);
        noInterrupts();

        RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0xF0);
        RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
        RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);    
        RPU_DataWrite(ADDRESS_U10_A, lampOutput);
      }
    }
#endif    

    // Latch 0xFF separately without interrupt clear
    RPU_DataWrite(ADDRESS_U10_A, 0xFF);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

    interrupts();
    noInterrupts();

    InsideZeroCrossingInterrupt = 0;
    RPU_DataWrite(ADDRESS_U10_A, backup10A);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, u10BControlLatest);

    // Read U10B to clear interrupt
    RPU_DataRead(ADDRESS_U10_B);
    numberOfU10Interrupts+=1;
  }
}




void RPU_HookInterrupts() {
  // Hook up the interrupt
/*
  cli();
  TCCR2A|=(1<<WGM21);     //Set the CTC mode
  OCR2A=0xBA;            //Set the value for 3ms
  TIMSK2|=(1<<OCIE2A);   //Set the interrupt request
  TCCR2B|=(1<<CS22);     //Set the prescale 1/64 clock
  sei();                 //Enable interrupt
*/  

  cli();
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for selected increment
  OCR1A = RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL;
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();
  
  attachInterrupt(digitalPinToInterrupt(2), InterruptService3, LOW);
}


boolean LookFor6800Activity() {
  // Assume Arduino pins all start as input
  unsigned long startTime = millis();
  boolean sawHigh = false;
  boolean sawLow = false;
  // for one second, look for activity on the VMA line (A5)
  // If we see anything, then the MPU is active so we shouldn't run
  while ((millis()-startTime)<1000) {
    if (PINC&0x20) sawHigh = true;
    else sawLow = true;
  }
  // If we saw both a high and low signal, then someone is toggling the 
  // VMA line, so we should hang here forever (until reset)
  if (sawHigh && sawLow) {
    return true;
  }
  return false;
}


void SetupArduinoPorts() {
#if (RPU_OS_HARDWARE_REV==1)
  // Arduino A0 = MPU A0
  // Arduino A1 = MPU A1
  // Arduino A2 = MPU A3
  // Arduino A3 = MPU A4
  // Arduino A4 = MPU A7
  // Arduino A5 = MPU VMA
  // Set up the address lines A0-A7 as output
  DDRC = DDRC | 0x3F;
  // Set up D13 as address line A5 (and set it low)
  DDRB = DDRB | 0x20;
  PORTB = PORTB & 0xDF;
  // Set up control lines & data lines
  DDRD = DDRD & 0xEB;
  DDRD = DDRD | 0xE8;
  // Set VMA OFF
  PORTC = PORTC & 0xDF;
  // Set R/W to HIGH
  PORTD = (PORTD | 0x08);  
#elif (RPU_OS_HARDWARE_REV==2) 
  // Set up the address lines A0-A7 as output
  DDRC = DDRC | 0x3F;
  // Set up D13 as address line A7 (and set it high)
  DDRB = DDRB | 0x20;
  PORTB = PORTB | 0x20;
  // Set up control lines & data lines
  DDRD = DDRD & 0xEB;
  DDRD = DDRD | 0xE8;
  // Set VMA OFF
  PORTC = PORTC & 0xDF;
  // Set R/W to HIGH
  PORTD = (PORTD | 0x08);  
#elif (RPU_OS_HARDWARE_REV==3) 
  pinMode(3, INPUT); // CLK
  pinMode(4, OUTPUT); // VMA
  pinMode(5, OUTPUT); // R/W
  for (byte count=6; count<13; count++) pinMode(count, INPUT); // D0-D6
  pinMode(13, INPUT); // Switch
  pinMode(14, OUTPUT); // Halt
  pinMode(15, INPUT); // D7
  for (byte count=16; count<32; count++) pinMode(count, OUTPUT); // Address lines are output
  digitalWrite(5, HIGH);  // Set R/W line high (Read)
  digitalWrite(4, LOW);  // Set VMA line LOW
#elif (RPU_OS_HARDWARE_REV==4)
#endif
  
}


boolean CheckCreditResetSwitchArch1(byte creditResetSwitch) {
  // Check for credit button
  InitializeU10PIA();
  InitializeU11PIA();

  byte strobeNum = 0x01 << (creditResetSwitch/8);
  byte switchNum = 0x01 << (creditResetSwitch%8);
  RPU_DataWrite(ADDRESS_U10_A, strobeNum);
  // Turn off U10:CB2 if it's on (because it strobes the last bank of dip switches
  RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x34);

  // Delay for switch capacitors to charge
  delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
      
  // Read the switches
  byte curSwitchByte = RPU_DataRead(ADDRESS_U10_B);

  //Unset the strobe
  RPU_DataWrite(ADDRESS_U10_A, 0x00);

  if (curSwitchByte & switchNum) {
    return true;
  }  
  return false;
}


unsigned long RPU_InitializeMPUArch1(unsigned long initOptions, byte creditResetSwitch) {
  unsigned long retResult = RPU_RET_NO_ERRORS;
  // Wait for board to boot
  delayMicroseconds(50000);
  delayMicroseconds(50000);

#if (RPU_OS_HARDWARE_REV==1) or (RPU_OS_HARDWARE_REV==2)
  (void)creditResetSwitch;
  if (LookFor6800Activity()) {
    if (initOptions&RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) {
      retResult |= RPU_RET_ORIGINAL_CODE_REQUESTED;
    } else {
      while (1);
    }
  }
  if (initOptions&( RPU_CMD_BOOT_ORIGINAL | RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_NEW_IF_CREDIT_RESET | 
                    RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED | RPU_CMD_AUTODETECT_ARCHITECTURE ) ) {
    retResult |= RPU_RET_OPTION_NOT_SUPPORTED;
  }
#elif (RPU_OS_HARDWARE_REV==3)
  (void)creditResetSwitch;

  if (initOptions&( RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_NEW_IF_CREDIT_RESET | 
                    RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED | RPU_CMD_AUTODETECT_ARCHITECTURE ) ) {
    retResult |= RPU_RET_OPTION_NOT_SUPPORTED;
  }

  pinMode(13, INPUT);
  boolean switchStateClosed = digitalRead(13) ? false : true;
  boolean bootToOriginal = false;
  if (  (initOptions & RPU_CMD_BOOT_ORIGINAL) || 
        (switchStateClosed && (initOptions&RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED)) ||
        (!switchStateClosed && (initOptions&RPU_CMD_BOOT_NEW_IF_SWITCH_CLOSED)) ) {
    bootToOriginal = true;
  }
  if ((initOptions & RPU_CMD_BOOT_NEW) || (switchStateClosed && (initOptions&RPU_CMD_BOOT_NEW_IF_SWITCH_CLOSED))) {
    bootToOriginal = false;
  }

  if (bootToOriginal) {
    // Let the 680X run 
    pinMode(14, OUTPUT); // Halt
    digitalWrite(14, HIGH);
    if (initOptions&RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) {
      retResult |= RPU_RET_ORIGINAL_CODE_REQUESTED;
    } else {
      while (1);
    }
  } else {
    // Switch indicates the Arduino should run, so HALT the 680X
    pinMode(14, OUTPUT); // Halt
    digitalWrite(14, LOW);
  }


#elif (RPU_OS_HARDWARE_REV==4) || (RPU_OS_HARDWARE_REV>=101)
  // put the 680X buffers into tri-state
  pinMode(RPU_BUFFER_DISABLE, OUTPUT);
  digitalWrite(RPU_BUFFER_DISABLE, 1);

  // Set /HALT low so the processor doesn't come online
  // (on some hardware, HALT & RESET are combined)
  pinMode(RPU_HALT_PIN, OUTPUT); 
  digitalWrite(RPU_HALT_PIN, 0);  
  pinMode(RPU_RESET_PIN, OUTPUT); 
  digitalWrite(RPU_RESET_PIN, 0);  

  // Set VMA, R/W to OUTPUT
  pinMode(RPU_VMA_PIN, OUTPUT);
  pinMode(RPU_RW_PIN, OUTPUT);
  RPU_SetAddressPinsDirection(RPU_PINS_OUTPUT);

#if (RPU_OS_HARDWARE_REV==102)
  if (CheckForMPUClock()) UsesM6800Processor = true;
  else UsesM6800Processor = false;
#endif

  // Set PHI2 depending on processor type
  if (UsesM6800Processor) {
    pinMode(RPU_PHI2_PIN, INPUT);
  } else {
    pinMode(RPU_PHI2_PIN, OUTPUT);
  }

  delay(1000);
//  RPU_DataWrite(ADDRESS_SB100, 0x01);
  boolean switchStateClosed = false;
  pinMode(RPU_SWITCH_PIN, INPUT);
  if (digitalRead(RPU_SWITCH_PIN)) {
    switchStateClosed = true;
    retResult |= RPU_RET_SELECTOR_SWITCH_ON;
  }

  boolean creditResetButtonHit = false;
  if ( creditResetSwitch!=0xFF && (initOptions & (RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_NEW_IF_CREDIT_RESET))) {
    // We have to check the credit/reset button to honor the init request
    creditResetButtonHit = CheckCreditResetSwitchArch1(creditResetSwitch);
    if (creditResetButtonHit) {
      retResult |= RPU_RET_CREDIT_RESET_BUTTON_HIT;
    }
  }

  boolean bootToOriginal = false;
  if (  (initOptions & RPU_CMD_BOOT_ORIGINAL) || 
        (switchStateClosed && (initOptions&RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED)) ||
        (!creditResetButtonHit && (initOptions&RPU_CMD_BOOT_NEW_IF_CREDIT_RESET)) ||
        (creditResetButtonHit && (initOptions&RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET)) ) {
    bootToOriginal = true;
  }
  if (  (initOptions & RPU_CMD_BOOT_NEW) || 
        (switchStateClosed && (initOptions&RPU_CMD_BOOT_NEW_IF_SWITCH_CLOSED)) ||
        (creditResetButtonHit && (initOptions&RPU_CMD_BOOT_NEW_IF_CREDIT_RESET)) ) {
    bootToOriginal = false;
  }

  if (bootToOriginal) {
    // If the options guide us to original code, boot to original
    pinMode(RPU_BUFFER_DISABLE, OUTPUT); // IRQ
    // Turn on the tri-state buffers
    digitalWrite(RPU_BUFFER_DISABLE, 0);
    
    pinMode(RPU_PHI2_PIN, INPUT); // CLOCK
    pinMode(RPU_VMA_PIN, INPUT); // VMA
    pinMode(RPU_RW_PIN, INPUT); // R/W

#if (RPU_OS_HARDWARE_REV==102)
    // We need to make sure the clock direction
    // buffers are set the correct direction
    if (UsesM6800Processor) {
      pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
      digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 0);
      pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
      digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
      retResult |= RPU_RET_6800_DETECTED;
    } else {
      pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
      digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 1);
      pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
      digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 0);
      retResult |= RPU_RET_6802_OR_8_DETECTED;
    }    
#endif
    // Set all the pins to input so they'll stay out of the way
    RPU_SetDataPinsDirection(RPU_PINS_INPUT);
    RPU_SetAddressPinsDirection(RPU_PINS_INPUT);

    // Set /HALT high
    pinMode(RPU_HALT_PIN, OUTPUT);
    digitalWrite(RPU_HALT_PIN, 1);
    pinMode(RPU_RESET_PIN, OUTPUT);
    digitalWrite(RPU_RESET_PIN, 1);

    retResult |= RPU_RET_ORIGINAL_CODE_REQUESTED;
    if (!(initOptions&RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN)) while (1);
    else return retResult;    
  }

#endif  

  SetupArduinoPorts();

  // Prep the address bus (all lines zero)
  RPU_DataRead(0);
  // Set up the PIAs
  InitializeU10PIA();
  InitializeU11PIA();

  // Read values from MPU dip switches
#ifdef RPU_OS_USE_DIP_SWITCHES  
  ReadDipSwitches();
#endif 

#if (RPU_OS_HARDWARE_REV==4) || (RPU_OS_HARDWARE_REV>100)
  pinMode(RPU_DIAGNOSTIC_PIN, INPUT);
  if (digitalRead(RPU_DIAGNOSTIC_PIN)==1) retResult |= RPU_RET_DIAGNOSTIC_REQUESTED;
#endif  
  
  // Reset address bus
  RPU_DataRead(0);
  RPU_ClearVariables();
  RPU_HookInterrupts();
  RPU_DataRead(0);  // Reset address bus

  // Clear all possible interrupts by reading the registers
  RPU_DataRead(ADDRESS_U11_A);
  RPU_DataRead(ADDRESS_U11_B);
  RPU_DataRead(ADDRESS_U10_A);
  RPU_DataRead(ADDRESS_U10_B);
  if (initOptions&RPU_CMD_PERFORM_MPU_TEST) retResult |= RPU_TestPIAs();
  RPU_DataRead(0);  // Reset address bus

  return retResult;
}

#endif 




#if (RPU_MPU_ARCHITECTURE>=10)

boolean CheckSwitchStack(byte switchNum) {
  for (byte stackIndex=SwitchStackFirst; stackIndex!=SwitchStackLast; stackIndex++) {
    if (stackIndex>=SWITCH_STACK_SIZE) stackIndex = 0; 
    if (SwitchStack[stackIndex]==switchNum) return true;
  }
  return false;
}


volatile unsigned long LampPass = 0;
volatile byte LampStrobe = 0;
volatile byte DisplayStrobe = 0;
volatile byte InterruptPass = 0;
boolean NeedToTurnOffTriggeredSolenoids = true;
#if (RPU_OS_NUM_DIGITS==6)
byte BlankingBit[16] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x01, 0x02, 0x01, 0x02, 0x04, 0x08, 0x010, 0x20, 0x01, 0x02};
#elif (RPU_OS_NUM_DIGITS==7) 
byte BlankingBit[16] = {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x02, 0x01, 0x02, 0x04, 0x08, 0x010, 0x20, 0x40};
#endif
volatile byte UpDownPassCounter = 0;

// INTERRUPT HANDLER
ISR(TIMER1_COMPA_vect) {    //This is the interrupt request (running at 965.3 Hz)

  byte displayControlPortB = RPU_DataRead(PIA_DISPLAY_CONTROL_B);
  if (displayControlPortB & 0x80) {
    UpDownSwitch = true;
    UpDownPassCounter = 0;
    // Clear the interrupt
    RPU_DataRead(PIA_DISPLAY_PORT_B);
  } else {
    UpDownPassCounter += 1;
    if (UpDownPassCounter==50) {
      UpDownSwitch = false;
      UpDownPassCounter = 0;
    }
  }

#if (RPU_MPU_ARCHITECTURE==15)
  // Create display data
  unsigned int digit1 = 0x0000;
  byte digit2 = 0x00;
  byte blankingBit = BlankingBit[DisplayStrobe];
  if (DisplayStrobe==0) {
    if (DisplayBIPDigitEnable&blankingBit) digit1 = DisplayBIPDigits[0];
    if (DisplayCreditDigitEnable&blankingBit) digit2 = DisplayCreditDigits[0];
  } else if (DisplayStrobe<8) {    
    if (DisplayDigitEnable[0]&blankingBit) digit1 = FourteenSegmentASCII[DisplayText[0][DisplayStrobe-1]];
    if (DisplayDigitEnable[2]&blankingBit) digit2 = DisplayDigits[2][DisplayStrobe-1];
  } else if (DisplayStrobe==8) {
    if (DisplayBIPDigitEnable&blankingBit) digit1 = DisplayBIPDigits[1];
    if (DisplayCreditDigitEnable&blankingBit) digit2 = DisplayCreditDigits[1];
  } else {
    if (DisplayDigitEnable[1]&blankingBit) digit1 = FourteenSegmentASCII[DisplayText[1][DisplayStrobe-9]];
    if (DisplayDigitEnable[3]&blankingBit) digit2 = DisplayDigits[3][DisplayStrobe-9];
  }
  // Show current display digit
  RPU_DataWrite(PIA_DISPLAY_PORT_A, BoardLEDs|DisplayStrobe);
  RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_A, (digit1>>7) & 0x7F);
  RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_B, digit1 & 0x7F);
  RPU_DataWrite(PIA_DISPLAY_PORT_B, digit2 & 0x7F);  
#elif (RPU_MPU_ARCHITECTURE==13)
  // Create display data
  byte digit1 = 0x0F, digit2 = 0x0F;
  byte blankingBit = BlankingBit[DisplayStrobe];
  if (DisplayStrobe==0) {
    if (DisplayBIPDigitEnable&blankingBit) digit1 = DisplayBIPDigits[0];
    if (DisplayCreditDigitEnable&blankingBit) digit2 = DisplayCreditDigits[0];
  } else if (DisplayStrobe<8) {
    if (DisplayDigitEnable[0]&blankingBit) digit1 = DisplayDigits[0][DisplayStrobe-1];
    if (DisplayDigitEnable[2]&blankingBit) digit2 = DisplayDigits[2][DisplayStrobe-1];
  } else if (DisplayStrobe==8) {
    if (DisplayBIPDigitEnable&blankingBit) digit1 = DisplayBIPDigits[1];
    if (DisplayCreditDigitEnable&blankingBit) digit2 = DisplayCreditDigits[1];
  } else {
    if (DisplayDigitEnable[1]&blankingBit) digit1 = DisplayDigits[1][DisplayStrobe-9];
    if (DisplayDigitEnable[3]&blankingBit) digit2 = DisplayDigits[3][DisplayStrobe-9];
  }
  // Show current display digit
  RPU_DataWrite(PIA_DISPLAY_PORT_A, BoardLEDs|DisplayStrobe);
  RPU_DataWrite(PIA_DISPLAY_PORT_B, digit1*16 | (digit2&0x0F));
#else
  // Create display data
  byte digit1 = 0x0F, digit2 = 0x0F;
  byte blankingBit = BlankingBit[DisplayStrobe];
  if (DisplayStrobe<6) {
    if (DisplayDigitEnable[0]&blankingBit) digit1 = DisplayDigits[0][DisplayStrobe];
    if (DisplayDigitEnable[2]&blankingBit) digit2 = DisplayDigits[2][DisplayStrobe];
  } else if (DisplayStrobe<8) {
    if (DisplayBIPDigitEnable&blankingBit) digit1 = DisplayBIPDigits[DisplayStrobe-6];
  } else if (DisplayStrobe<14) {
    if (DisplayDigitEnable[1]&blankingBit) digit1 = DisplayDigits[1][DisplayStrobe-8];
    if (DisplayDigitEnable[3]&blankingBit) digit2 = DisplayDigits[3][DisplayStrobe-8];
  } else {
    if (DisplayCreditDigitEnable&blankingBit) digit1 = DisplayCreditDigits[DisplayStrobe-14];
  }  
  // Show current display digit
//  if (RPU_DataRead(PIA_DISPLAY_CONTROL_B) & 0x80) SawInterruptOnDisplayPortB1 = true;
  RPU_DataWrite(PIA_DISPLAY_PORT_A, BoardLEDs|DisplayStrobe);
  RPU_DataWrite(PIA_DISPLAY_PORT_B, digit1*16 | (digit2&0x0F));
#endif

  DisplayStrobe += 1; 
  if (DisplayStrobe>=16) DisplayStrobe = 0;

  if (InterruptPass==0) {
  
    // Show lamps
    byte curLampByte = LampStates[LampStrobe];
    if (LampPass%DimDivisor1) curLampByte |= LampDim1[LampStrobe];
    if (LampPass%DimDivisor2) curLampByte |= LampDim2[LampStrobe];
    RPU_DataWrite(PIA_LAMPS_PORT_B, 0x01<<(LampStrobe));
    RPU_DataWrite(PIA_LAMPS_PORT_A, curLampByte);
    
    LampStrobe += 1;
    if ((LampStrobe)>=RPU_NUM_LAMP_BANKS) {
      LampStrobe = 0;
      LampPass += 1;
    }
    
    // Check coin door switches
    byte displayControlPortA = RPU_DataRead(PIA_DISPLAY_CONTROL_A);
    if (displayControlPortA & 0x80) {
      // If the diagnostic switch isn't on the stack already, put it there
      if (!CheckSwitchStack(SW_SELF_TEST_SWITCH)) PushToSwitchStack(SW_SELF_TEST_SWITCH);
      // Clear the interrupt
      RPU_DataRead(PIA_DISPLAY_PORT_A);
    }

    // Check switches
    byte switchColStrobe = 1;
    for (byte switchCol=0; switchCol<8; switchCol++) {
      // Cycle the debouncing variables
      SwitchesMinus2[switchCol] = SwitchesMinus1[switchCol];
      SwitchesMinus1[switchCol] = SwitchesNow[switchCol];
      // Turn on the strobe
      RPU_DataWrite(PIA_SWITCH_PORT_B, switchColStrobe);
      // Hold it up for 30 us
      delayMicroseconds(12);
      // Read switch input
      SwitchesNow[switchCol] = RPU_DataRead(PIA_SWITCH_PORT_A);
      switchColStrobe *= 2;
    }
    RPU_DataWrite(PIA_SWITCH_PORT_B, 0);
    
    // If there are any closures, add them to the switch stack
    for (byte switchCol=0; switchCol<NUM_SWITCH_BYTES; switchCol++) {
      byte validClosures = (SwitchesNow[switchCol] & SwitchesMinus1[switchCol]) & ~SwitchesMinus2[switchCol];
      // If there is a valid switch closure (off, on, on)
      if (validClosures) {
        // Loop on bits of switch byte
        for (byte bitCount=0; bitCount<8; bitCount++) {
          // If this switch bit is closed
          if (validClosures&0x01) {
            byte validSwitchNum = switchCol*8 + bitCount;
            PushToSwitchStack(validSwitchNum);
          }
          validClosures = validClosures>>1;
        }        
      }
    }
  
  } else {
    // See if any solenoids need to be switched
    byte solenoidOn = PullFirstFromSolenoidStack();
    byte portA = ContinuousSolenoidBits&0xFF;
    byte portB = ContinuousSolenoidBits/256;
    if (solenoidOn!=SOLENOID_STACK_EMPTY) {
      if (solenoidOn<16) {
        unsigned short newSolenoidBytes = (1<<solenoidOn);
        portA |= (newSolenoidBytes&0xFF);
        portB |= (newSolenoidBytes/256);
        if (NeedToTurnOffTriggeredSolenoids) {
          RPU_DataWrite(PIA_LAMPS_CONTROL_B, 0x3C);
          RPU_DataWrite(PIA_LAMPS_CONTROL_A, 0x3C);
          RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x3C);
          RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x3C);
          RPU_DataWrite(PIA_SOLENOID_CONTROL_A, 0x3C);
          RPU_DataWrite(PIA_DISPLAY_CONTROL_B, 0x3D);
          NeedToTurnOffTriggeredSolenoids = false;
        }
      } else {
        if (solenoidOn==16) RPU_DataWrite(PIA_LAMPS_CONTROL_B, 0x34);
        if (solenoidOn==17) RPU_DataWrite(PIA_LAMPS_CONTROL_A, 0x34);
        if (solenoidOn==18) RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x34);
        if (solenoidOn==19) RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x34);
        if (solenoidOn==20) RPU_DataWrite(PIA_SOLENOID_CONTROL_A, 0x34);
        if (solenoidOn==21) RPU_DataWrite(PIA_DISPLAY_CONTROL_B, 0x35);
        NeedToTurnOffTriggeredSolenoids = true;
      }
    } else if (NeedToTurnOffTriggeredSolenoids) {
      NeedToTurnOffTriggeredSolenoids = false;
      RPU_DataWrite(PIA_LAMPS_CONTROL_B, 0x3C);
      RPU_DataWrite(PIA_LAMPS_CONTROL_A, 0x3C);
      RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x3C);
      RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x3C);
      RPU_DataWrite(PIA_SOLENOID_CONTROL_A, 0x3C);
      RPU_DataWrite(PIA_DISPLAY_CONTROL_B, 0x3D);
    }

  
#if defined(RPU_OS_USE_WTYPE_1_SOUND)
    // See if any sounds need to be added
    // (these are handled through solenoid lines)
    unsigned short soundOn = PullFirstFromSoundStack();
    if (soundOn!=SOUND_STACK_EMPTY) {
      portA |= (soundOn&0xFF);
      portB |= (soundOn/256);
    }
#elif defined(RPU_OS_USE_WTYPE_2_SOUND)
    unsigned short soundOn = PullFirstFromSoundStack();
    if (soundOn!=SOUND_STACK_EMPTY) {
      RPU_DataWrite(PIA_SOUND_COMMA_PORT_A, (~soundOn) & 0x7F);      
    } else {
      RPU_DataWrite(PIA_SOUND_COMMA_PORT_A, 0x7F);
    }
#endif    

    RPU_DataWrite(PIA_SOLENOID_PORT_A, portA);
#if (RPU_MPU_ARCHITECTURE==15)
    RPU_DataWrite(PIA_SOLENOID_11_PORT_B, portB);
#else 
    RPU_DataWrite(PIA_SOLENOID_PORT_B, portB);
#endif    
  }

//  RPU_DataWrite(PIA_SOLENOID_11_PORT_B, InterruptPass);
  InterruptPass ^= 1;

}



void RPU_SetupInterrupt() {
  cli();
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for selected increment
//  OCR1A = 16574;
  OCR1A = INTERRUPT_OCR1A_COUNTER;
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();
}


boolean RPU_DiagnosticModeRequested() {
  boolean bootToDiagnostics = false;
#if (RPU_OS_HARDWARE_REV==4) || (RPU_OS_HARDWARE_REV>100)
  pinMode(RPU_DIAGNOSTIC_PIN, INPUT);
  if (digitalRead(RPU_DIAGNOSTIC_PIN)==1) bootToDiagnostics = true;
#endif  

  return bootToDiagnostics;
}


boolean CheckCreditResetSwitchArch10(byte creditResetButton) {
  byte strobeLine = 0x01 << (creditResetButton/8);
  byte returnLine = 0x01 << (creditResetButton%8);

  RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x38);
  RPU_DataWrite(PIA_SWITCH_PORT_A, 0x00);
  RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x3C);

  RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x38);
  RPU_DataWrite(PIA_SWITCH_PORT_B, 0xFF);
  RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x3C);
  RPU_DataWrite(PIA_SWITCH_PORT_B, 0x00);

  RPU_DataWrite(PIA_SWITCH_PORT_B, strobeLine);
  // Hold it up for 30 us
  delayMicroseconds(12);

  // Read switch input
  byte switchValues = RPU_DataRead(PIA_SWITCH_PORT_A);
  RPU_DataWrite(PIA_SWITCH_PORT_B, 0);

  if (switchValues & returnLine) return true;
  return false;
}    

/*****************************************************
 *  Initialization for Architecture 10 or greater
 */

unsigned long RPU_InitializeMPUArch10(unsigned long initOptions, byte creditResetSwitch) {
  unsigned long retResult = RPU_RET_NO_ERRORS;

  // put the 680X buffers into tri-state
  pinMode(RPU_BUFFER_DISABLE, OUTPUT);
  digitalWrite(RPU_BUFFER_DISABLE, 1);

  // Set /HALT low so the processor doesn't come online
  // (on some hardware, HALT & RESET are combined)
  pinMode(RPU_HALT_PIN, OUTPUT); 
  digitalWrite(RPU_HALT_PIN, 0);  
  pinMode(RPU_RESET_PIN, OUTPUT); 
  digitalWrite(RPU_RESET_PIN, 0); 

  // Determine if we can detect a 
  // 6800 or 6802/8 and possibly override
  // value for UsesM6800Processor
#if (RPU_OS_HARDWARE_REV==102)
  if (CheckForMPUClock()) UsesM6800Processor = true;
  else UsesM6800Processor = false;
#endif

  // Set VMA, R/W, and PHI2 to OUTPUT
  pinMode(RPU_VMA_PIN, OUTPUT);
  pinMode(RPU_RW_PIN, OUTPUT);
  if (!UsesM6800Processor) pinMode(RPU_PHI2_PIN, OUTPUT);
  else pinMode(RPU_PHI2_PIN, INPUT);
  // Make sure PIA IV (solenoid) CB2 is off so that solenoids are off
  RPU_SetAddressPinsDirection(RPU_PINS_OUTPUT);  
  RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x30);
  
  delay(1000);
  boolean switchStateClosed = false;
  pinMode(RPU_SWITCH_PIN, INPUT);
  if (digitalRead(RPU_SWITCH_PIN)) {
    switchStateClosed = true;
    retResult |= RPU_RET_SELECTOR_SWITCH_ON;
  }

  boolean creditResetButtonHit = false;
  if ( creditResetSwitch!=0xFF && (initOptions & (RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_NEW_IF_CREDIT_RESET))) {
    // We have to check the credit/reset button to honor the init request
    creditResetButtonHit = CheckCreditResetSwitchArch10(creditResetSwitch);
    if (creditResetButtonHit) {
      retResult |= RPU_RET_CREDIT_RESET_BUTTON_HIT;
    }
  }

  boolean bootToOriginal = false;
  if (  (initOptions & RPU_CMD_BOOT_ORIGINAL) || 
        (switchStateClosed && (initOptions&RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED)) ||
        (!creditResetButtonHit && (initOptions&RPU_CMD_BOOT_NEW_IF_CREDIT_RESET)) ||
        (creditResetButtonHit && (initOptions&RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET)) ) {
    bootToOriginal = true;
  }
  if (  (initOptions & RPU_CMD_BOOT_NEW) || 
        (switchStateClosed && (initOptions&RPU_CMD_BOOT_NEW_IF_SWITCH_CLOSED)) ||
        (creditResetButtonHit && (initOptions&RPU_CMD_BOOT_NEW_IF_CREDIT_RESET)) ) {
    bootToOriginal = false;
  }

  if (bootToOriginal) {
    // If the switch is off, allow 6808 to boot
    pinMode(RPU_BUFFER_DISABLE, OUTPUT);
    // Turn on the tri-state buffers
    digitalWrite(RPU_BUFFER_DISABLE, 0);
    
    pinMode(RPU_PHI2_PIN, INPUT); // CLOCK
    pinMode(RPU_VMA_PIN, INPUT); // VMA
    pinMode(RPU_RW_PIN, INPUT); // R/W

#if (RPU_OS_HARDWARE_REV==102)
    // We need to make sure the clock direction
    // buffers are set the correct direction
    if (UsesM6800Processor) {
      pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
      digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 0);
      pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
      digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
      retResult |= RPU_RET_6800_DETECTED;
    } else {
      pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
      digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 1);
      pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
      digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 0);
      retResult |= RPU_RET_6802_OR_8_DETECTED;
    }    
#endif

    // Set all the pins to input so they'll stay out of the way
    RPU_SetDataPinsDirection(RPU_PINS_INPUT);
    RPU_SetAddressPinsDirection(RPU_PINS_INPUT);

    // Set /HALT high
    pinMode(RPU_HALT_PIN, OUTPUT);
    digitalWrite(RPU_HALT_PIN, 1);
    pinMode(RPU_RESET_PIN, OUTPUT);
    digitalWrite(RPU_RESET_PIN, 1);

    if (initOptions&RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) {
      retResult |= RPU_RET_ORIGINAL_CODE_REQUESTED;
      return retResult;
    } else {
      while (1);
    }
  }
  
#if (RPU_OS_HARDWARE_REV>100)
  pinMode(RPU_DIAGNOSTIC_PIN, INPUT);
  if (digitalRead(RPU_DIAGNOSTIC_PIN)==1) retResult |= RPU_RET_DIAGNOSTIC_REQUESTED;
#endif  
  
  RPU_ClearVariables();
  RPU_SetAddressPinsDirection(RPU_PINS_OUTPUT);
  RPU_InitializePIAs();
  if (initOptions&RPU_CMD_PERFORM_MPU_TEST) retResult |= RPU_TestPIAs();
  RPU_SetupInterrupt();

  return retResult;
}


#endif

void RPU_Update(unsigned long currentTime) {
  RPU_ApplyFlashToLamps(currentTime);
  RPU_UpdateTimedSolenoidStack(currentTime);
#if (RPU_MPU_ARCHITECTURE>=10) && (defined(RPU_OS_USE_WTYPE_1_SOUND) || defined(RPU_OS_USE_WTYPE_2_SOUND))
  RPU_UpdateTimedSoundStack(currentTime);
#endif
}


// This function should eventually support auto-detect and initialize the appropriate
// ISRs for the detected architecture.
unsigned long RPU_InitializeMPU(unsigned long initOptions, byte creditResetSwitch) {

  unsigned long retVal = 0;

#if (RPU_MPU_ARCHITECTURE<10)
  retVal = RPU_InitializeMPUArch1(initOptions, creditResetSwitch);
#else
  retVal = RPU_InitializeMPUArch10(initOptions, creditResetSwitch);
#endif  

  return retVal;
}
