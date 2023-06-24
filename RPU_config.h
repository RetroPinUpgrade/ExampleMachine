/**************************************************************************
 *     This file is part of the RPU OS for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    RPU OS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPU OS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#ifndef RPU_CONFIG_H

/***

  Use this file to set game-specific and hardware-specific parameters

***/
// Plug-in board architectures (0-99 is for J5, 100-199 is for CPU socket)
// Hardware Rev 1 generally uses an Arduino Nano & (optional) 74125
// Hardware Rev 2 uses an Arduino Nano, a 74155, and a 74240
// Hardware Rev 3 uses a MEGA 2560 Pro, and nothing else
// Hardware Rev 4 uses a MEGA 2560 Pro (all the pins) on a larger board (display & WIFI)
// Hardware Rev 100 (different order of magnitude because it's a different approach) plugs into the CPU socket
// Hardware Rev 101 - first RPU CPU interposer release board
// Hardware Rev 102 - second RPU (with display and WIFI socket)
#define RPU_OS_HARDWARE_REV   3

// Available Architectures (0-9 is for B/S Boards, 10-19 is for W)
//  RPU_MPU_ARCHITECTURE 1 = -17, -35, 100, 200, or compatible
//  RPU_MPU_ARCHITECTURE 11 = Sys 4, 6
//  RPU_MPU_ARCHITECTURE 13 = Sys 7
//  RPU_MPU_ARCHITECTURE 15 = Sys 11
#define RPU_MPU_ARCHITECTURE  1

// Some boards will assume a 6800 is the processor (RPU_OS_HARDWARE_REV 1 through 4)
// and some boards will try to detect the processor (RPU_OS_HARDWARE_REV 102)
// but in other cases we can specify if we're building for a 6800.
// Define RPU_MPU_BUILD_FOR_6800 with a 0 for 6802 or 6808, and with
// a 1 for 6800
#define RPU_MPU_BUILD_FOR_6800  1

// These defines allow this configuration to eliminate some functions
// to reduce program size
//#define RPU_OS_USE_DIP_SWITCHES 
//#define RPU_OS_USE_S_AND_T
//#define RPU_OS_USE_DASH51
//#define RPU_OS_USE_SB100
//#define RPU_OS_USE_SB300
//#define RPU_OS_USE_WAV_TRIGGER
//#define RPU_OS_USE_WAV_TRIGGER_1p3
//#define RPU_OS_DISABLE_CPC_FOR_SPACE
//#define RPU_OS_USE_AUX_LAMPS
//#define RPU_OS_USE_7_DIGIT_DISPLAYS
//#define RPU_USE_EXTENDED_SWITCHES_ON_PB4
//#define RPU_USE_EXTENDED_SWITCHES_ON_PB7
//#define RPU_OS_USE_WTYPE_1_SOUND
//#define RPU_OS_USE_WTYPE_2_SOUND
//#define RPU_OS_USE_W11_SOUND




#if (RPU_MPU_ARCHITECTURE==1) 
/*******************************************************
 * This section is only for games that use the 
 * -17, -35, 100, and 200 MPU boards
 */
// Depending on the number of digits, the RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL
// can be adjusted in order to change the refresh rate of the displays.
// The original -17 / MPU-100 boards ran at 320 Hz 
// The Alltek runs the displays at 440 Hz (probably so 7-digit displays won't flicker)
// The value below is calculated with this formula:
//       Value = (interval in ms) * (16*10^6) / (1*1024) - 1 
//          (must be <65536)
// Choose one of these values (or do whatever)
//  Value         Frequency 
//  48            318.8 Hz
//  47            325.5 Hz
//  46            332.4 Hz increments   (I use this for 6-digits displays)
//  45            339.6 Hz
//  40            381 Hz
//  35            434 Hz     (This would probably be good for 7-digit displays)
//  34            446.4 Hz      
#define RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL  48  
//#define RPU_OS_ADJUSTABLE_DISPLAY_INTERRUPT

#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
#define RPU_OS_MASK_SHIFT_1            0x60
#define RPU_OS_MASK_SHIFT_2            0x0C
#else
#define RPU_OS_MASK_SHIFT_1            0x30
#define RPU_OS_MASK_SHIFT_2            0x06
#endif

#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
#define RPU_OS_MAX_DISPLAY_SCORE  9999999
#define RPU_OS_NUM_DIGITS         7
#define RPU_OS_ALL_DIGITS_MASK    0x7F
#else
#define RPU_OS_MAX_DISPLAY_SCORE  999999
#define RPU_OS_NUM_DIGITS         6
#define RPU_OS_ALL_DIGITS_MASK    0x3F
#endif

#define CONTSOL_DISABLE_FLIPPERS      0x40
#define CONTSOL_DISABLE_COIN_LOCKOUT  0x20

#define RPU_OS_SWITCH_DELAY_IN_MICROSECONDS 200
#define RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS  70

// Fast boards might need a slower lamp strobe
#define RPU_OS_SLOW_DOWN_LAMP_STROBE  0

#ifdef RPU_OS_USE_AUX_LAMPS
#define RPU_NUM_LAMP_BANKS 11
#define RPU_MAX_LAMPS      88
#else
#define RPU_NUM_LAMP_BANKS 8
#define RPU_MAX_LAMPS      60
#endif 

#elif (RPU_MPU_ARCHITECTURE > 9) 
/*******************************************************
 * This section is only for games that use the 
 * System 4, 6, 7, 9 or 11 / other MPU boards
 */

#define RPU_NUM_LAMP_BANKS             8
#define RPU_MAX_LAMPS                  64

#define NUM_SWITCH_BYTES                8
#define MAX_NUM_SWITCHES                64
 
#if (RPU_MPU_ARCHITECTURE<13)
#define RPU_OS_MAX_DISPLAY_SCORE  999999
#define RPU_OS_NUM_DIGITS       6
#define RPU_OS_ALL_DIGITS_MASK    0x3F
#else
#define RPU_OS_MAX_DISPLAY_SCORE  9999999
#define RPU_OS_NUM_DIGITS       7
#define RPU_OS_ALL_DIGITS_MASK    0x7F
#endif

#endif


#define RPU_CREDITS_EEPROM_BYTE                   5
#define RPU_HIGHSCORE_EEPROM_START_BYTE           1
#define RPU_AWARD_SCORE_1_EEPROM_START_BYTE       10
#define RPU_AWARD_SCORE_2_EEPROM_START_BYTE       14
#define RPU_AWARD_SCORE_3_EEPROM_START_BYTE       18
#define RPU_TOTAL_PLAYS_EEPROM_START_BYTE         26
#define RPU_TOTAL_REPLAYS_EEPROM_START_BYTE       30
#define RPU_TOTAL_HISCORE_BEATEN_START_BYTE       34
#define RPU_CHUTE_2_COINS_START_BYTE              38
#define RPU_CHUTE_1_COINS_START_BYTE              42
#define RPU_CHUTE_3_COINS_START_BYTE              46
#define RPU_CPC_CHUTE_1_SELECTION_BYTE            50
#define RPU_CPC_CHUTE_2_SELECTION_BYTE            51
#define RPU_CPC_CHUTE_3_SELECTION_BYTE            52

#define RPU_CONFIG_H
#endif
