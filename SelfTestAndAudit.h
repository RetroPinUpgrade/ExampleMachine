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


#ifndef SELF_TEST_H
#define SELF_TEST_H

#if (RPU_MPU_ARCHITECTURE<10)
#define MACHINE_STATE_TEST_LAMPS          -1
#define MACHINE_STATE_TEST_DISPLAYS       -2
#define MACHINE_STATE_TEST_SOLENOIDS      -3
#define MACHINE_STATE_TEST_SWITCHES       -4
#define MACHINE_STATE_TEST_SOUNDS         -5
#define MACHINE_STATE_TEST_SCORE_LEVEL_1  -6
#define MACHINE_STATE_TEST_SCORE_LEVEL_2  -7
#define MACHINE_STATE_TEST_SCORE_LEVEL_3  -8
#define MACHINE_STATE_TEST_HISCR          -9
#define MACHINE_STATE_TEST_CREDITS        -10
#define MACHINE_STATE_TEST_TOTAL_PLAYS    -11
#define MACHINE_STATE_TEST_TOTAL_REPLAYS  -12
#define MACHINE_STATE_TEST_HISCR_BEAT     -13
#define MACHINE_STATE_TEST_CHUTE_2_COINS  -14
#define MACHINE_STATE_TEST_CHUTE_1_COINS  -15
#define MACHINE_STATE_TEST_CHUTE_3_COINS  -16
#define MACHINE_STATE_TEST_BOOT           -17
#else
#define MACHINE_STATE_TEST_SWITCHES       -1
#define MACHINE_STATE_TEST_SOLENOIDS      -2
#define MACHINE_STATE_TEST_SOUNDS         -3
#define MACHINE_STATE_TEST_LAMPS          -4
#define MACHINE_STATE_TEST_DISPLAYS       -5
#define MACHINE_STATE_TEST_BOOT           -6
#define MACHINE_STATE_TEST_HISCR          -7
#define MACHINE_STATE_TEST_SCORE_LEVEL_1  -8
#define MACHINE_STATE_TEST_SCORE_LEVEL_2  -9
#define MACHINE_STATE_TEST_SCORE_LEVEL_3  -10
#define MACHINE_STATE_TEST_CREDITS        -11
#define MACHINE_STATE_TEST_TOTAL_PLAYS    -12
#define MACHINE_STATE_TEST_TOTAL_REPLAYS  -13
#define MACHINE_STATE_TEST_HISCR_BEAT     -14
#define MACHINE_STATE_TEST_CHUTE_2_COINS  -15
#define MACHINE_STATE_TEST_CHUTE_1_COINS  -16
#define MACHINE_STATE_TEST_CHUTE_3_COINS  -17
#endif 

#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE  
#define MACHINE_STATE_ADJUST_CPC_CHUTE_1        -18
#define MACHINE_STATE_ADJUST_CPC_CHUTE_2        -19 
#define MACHINE_STATE_ADJUST_CPC_CHUTE_3        -20
// This define is set to the last test, so the extended settings will know when to take over
#define MACHINE_STATE_TEST_DONE           -20
#else
#define MACHINE_STATE_TEST_DONE           -17
#endif

unsigned long GetLastSelfTestChangedTime();
void SetLastSelfTestChangedTime(unsigned long setSelfTestChange);
int RunBaseSelfTest(int curState, boolean curStateChanged, unsigned long CurrentTime, byte resetSwitch, byte slamSwitch=0xFF);

unsigned long GetAwardScore(byte level);
#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE
byte GetCPCSelection(byte chuteNumber);
byte GetCPCCoins(byte cpcSelection);
byte GetCPCCredits(byte cpcSelection);
#endif
#endif
