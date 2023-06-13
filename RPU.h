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


#ifndef RPU_OS_H

#define RPU_OS_MAJOR_VERSION  5
#define RPU_OS_MINOR_VERSION  2

struct PlayfieldAndCabinetSwitch {
  byte switchNum;
  byte solenoid;
  byte solenoidHoldTime;
};

#define SW_SELF_TEST_SWITCH 0x7F
#define SOL_NONE 0x0F
#define SWITCH_STACK_EMPTY  0xFF
#define CONTSOL_DISABLE_FLIPPERS      0x40
#define CONTSOL_DISABLE_COIN_LOCKOUT  0x20


#define RPU_CMD_BOOT_ORIGINAL                   0x0001
#define RPU_CMD_BOOT_NEW                        0x0002
#define RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET   0x0004
#define RPU_CMD_BOOT_NEW_IF_CREDIT_RESET        0x0008
#define RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED  0x0010
#define RPU_CMD_BOOT_NEW_IF_SWITCH_CLOSED       0x0020
#define RPU_CMD_AUTODETECT_ARCHITECTURE         0x0040
#define RPU_CMD_PERFORM_MPU_TEST                0x0080

// If the caller chooses this option, it's up to them
// to honor the RPU_RET_ORIGINAL_CODE_REQUESTED return
// flag and halt the Arduino with a while(1);
#define RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN  0x0100

#define RPU_RET_NO_ERRORS                 0
#define RPU_RET_U10_PIA_ERROR             0x0001
#define RPU_RET_U11_PIA_ERROR             0x0002
#define RPU_RET_PIA_1_ERROR               0x0004
#define RPU_RET_PIA_2_ERROR               0x0008
#define RPU_RET_PIA_3_ERROR               0x0010
#define RPU_RET_PIA_4_ERROR               0x0020
#define RPU_RET_PIA_5_ERROR               0x0040
#define RPU_RET_OPTION_NOT_SUPPORTED      0x0080
#define RPU_RET_6800_DETECTED             0x0100
#define RPU_RET_6802_OR_8_DETECTED        0x0200
#define RPU_RET_DIAGNOSTIC_REQUESTED      0x1000
#define RPU_RET_SELECTOR_SWITCH_ON        0x2000
#define RPU_RET_CREDIT_RESET_BUTTON_HIT   0x4000
#define RPU_RET_ORIGINAL_CODE_REQUESTED   0x8000

// Function Prototypes

//   Initialization
unsigned long RPU_InitializeMPU(unsigned long initOptions=RPU_CMD_BOOT_NEW_IF_SWITCH_CLOSED|RPU_CMD_PERFORM_MPU_TEST, byte creditResetSwitch=0xFF);
void RPU_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches, PlayfieldAndCabinetSwitch *s_gameSwitchArray);
byte RPU_GetDipSwitches(byte index);

// EEProm Helper Functions
byte RPU_ReadByteFromEEProm(unsigned short startByte);
void RPU_WriteByteToEEProm(unsigned short startByte, byte value);
unsigned long RPU_ReadULFromEEProm(unsigned short startByte, unsigned long defaultValue=0);
void RPU_WriteULToEEProm(unsigned short startByte, unsigned long value);

//   Swtiches
byte RPU_PullFirstFromSwitchStack();
boolean RPU_ReadSingleSwitchState(byte switchNum);
void RPU_PushToSwitchStack(byte switchNumber);
boolean RPU_GetUpDownSwitchState(); // This always returns true for RPU_MPU_ARCHITECTURE==1 (no up/down switch)
void RPU_ClearUpDownSwitchState();

//   Solenoids
void RPU_PushToSolenoidStack(byte solenoidNumber, byte numPushes, boolean disableOverride = false);
void RPU_SetCoinLockout(boolean lockoutOff = false, byte solbit = CONTSOL_DISABLE_COIN_LOCKOUT);
void RPU_SetDisableFlippers(boolean disableFlippers = true, byte solbit = CONTSOL_DISABLE_FLIPPERS);
void RPU_SetContinuousSolenoidBit(boolean bitOn, byte solBit = 0x10);
#if (RPU_MPU_ARCHITECTURE>=10)
void RPU_SetContinuousSolenoid(boolean solOn, byte solNum);
#endif
boolean RPU_FireContinuousSolenoid(byte solBit, byte numCyclesToFire);
byte RPU_ReadContinuousSolenoids();
void RPU_DisableSolenoidStack();
void RPU_EnableSolenoidStack();
boolean RPU_PushToTimedSolenoidStack(byte solenoidNumber, byte numPushes, unsigned long whenToFire, boolean disableOverride = false);
void RPU_UpdateTimedSolenoidStack(unsigned long curTime);

//   Displays
byte RPU_SetDisplay(int displayNumber, unsigned long value, boolean blankByMagnitude=false, byte minDigits=2);
void RPU_SetDisplayBlank(int displayNumber, byte bitMask);
void RPU_SetDisplayCredits(int value, boolean displayOn = true, boolean showBothDigits=true);
void RPU_SetDisplayMatch(int value, boolean displayOn = true, boolean showBothDigits=true);
void RPU_SetDisplayBallInPlay(int value, boolean displayOn = true, boolean showBothDigits=true);
void RPU_SetDisplayFlash(int displayNumber, unsigned long value, unsigned long curTime, int period=500, byte minDigits=2);
void RPU_SetDisplayFlashCredits(unsigned long curTime, int period=100);
void RPU_CycleAllDisplays(unsigned long curTime, byte digitNum=0); // Self-test function
byte RPU_GetDisplayBlank(int displayNumber);
#if (RPU_MPU_ARCHITECTURE==15)
byte RPU_SetDisplayText(int displayNumber, char *text, boolean blankByLength=true);
#endif
#if defined(RPU_OS_ADJUSTABLE_DISPLAY_INTERRUPT)
void RPU_SetDisplayRefreshConstant(int intervalConstant);
#endif

//   Lamps
void RPU_SetLampState(int lampNum, byte s_lampState, byte s_lampDim=0, int s_lampFlashPeriod=0);
void RPU_ApplyFlashToLamps(unsigned long curTime);
void RPU_FlashAllLamps(unsigned long curTime); // Self-test function
void RPU_TurnOffAllLamps();
void RPU_SetDimDivisor(byte level=1, byte divisor=2); // 2 means 50% duty cycle, 3 means 33%, 4 means 25%...
byte RPU_ReadLampState(int lampNum);
byte RPU_ReadLampDim(int lampNum);
int RPU_ReadLampFlash(int lampNum);

// Sound Functions
#ifdef RPU_OS_USE_S_AND_T
void RPU_PlaySoundSAndT(byte soundByte);
#endif

#ifdef RPU_OS_USE_SB100
void RPU_PlaySB100(byte soundByte);
#if (RPU_OS_HARDWARE_REV==2)
void RPU_PlaySB100Chime(byte soundByte);
#endif 
#endif

#ifdef RPU_OS_USE_DASH51
void RPU_PlaySoundDash51(byte soundByte);
#endif

#if (RPU_OS_HARDWARE_REV>=2 && defined(RPU_OS_USE_SB300))
void RPU_PlaySB300SquareWave(byte soundRegister, byte soundByte);
void RPU_PlaySB300Analog(byte soundRegister, byte soundByte);
#endif 

#if defined(RPU_OS_USE_WTYPE_1_SOUND) || defined(RPU_OS_USE_WTYPE_2_SOUND)
void RPU_SetSoundValueLimits(unsigned short lowerLimit, unsigned short upperLimit);
void RPU_PushToSoundStack(unsigned short soundNumber, byte numPushes);
boolean RPU_PushToTimedSoundStack(unsigned short soundNumber, byte numPushes, unsigned long whenToPlay);
void RPU_UpdateTimedSoundStack(unsigned long curTime);
#endif
#ifdef RPU_OS_USE_WTYPE_11_SOUND
void RPU_PlayW11Sound(byte soundNum);
void RPU_PlayW11Music(byte songNum);
#endif


//   General
byte RPU_DataRead(int address);
void RPU_Update(unsigned long currentTime);

#ifdef RPU_CPP_FILE
  int NumGameSwitches = 0;
  int NumGamePrioritySwitches = 0;
  PlayfieldAndCabinetSwitch *GameSwitches = NULL;

#if (RPU_MPU_ARCHITECTURE==15)

// Alpha numeric numbers and alphabet

const uint16_t SevenSegmentNumbers[10] = {
  0x3F, /* 0 */
  0x06, /* 1 */
  0x5B, /* 2 */
  0x4F, /* 3 */
  0x66, /* 4 */
  0x6D, /* 5 */
  0x7D, /* 6 */
  0x07, /* 7 */
  0x7F, /* 8 */
  0x6F  /* 9 */
};

// alphanumeric 14-segment display (ASCII)
const uint16_t FourteenSegmentASCII[96] = {
  0x0000,/*   converted 0x0000 to 0x0000*/
  0x0006,/* ! converted 0x4006 to 0x0006*/
  0x0102,/* " converted 0x0202 to 0x0102*/
  0x154E,/* # converted 0x12CE to 0x154E*/
  0x156D,/* $ converted 0x12ED to 0x156D*/
  0x3FE4,/* % converted 0x3FE4 to 0x3FE4*/
  0x09D9,/* & converted 0x2359 to 0x09D9*/
  0x0100,/* ' converted 0x0200 to 0x0100*/
  0x0A00,/* ( converted 0x2400 to 0x0A00*/
  0x2080,/* ) converted 0x0900 to 0x2080*/
  0x3FC0,/* * converted 0x3FC0 to 0x3FC0*/
  0x1540,/* + converted 0x12C0 to 0x1540*/
  0x2000,/* , converted 0x0800 to 0x2000*/
  0x0440,/* - converted 0x00C0 to 0x0440*/
  0x0000,/* . converted 0x4000 to 0x0000*/
  0x2200,/* / converted 0x0C00 to 0x2200*/
  0x223F,/* 0 converted 0x0C3F to 0x223F*/
  0x0206,/* 1 converted 0x0406 to 0x0206*/
  0x045B,/* 2 converted 0x00DB to 0x045B*/
  0x040F,/* 3 converted 0x008F to 0x040F*/
  0x0466,/* 4 converted 0x00E6 to 0x0466*/
  0x0869,/* 5 converted 0x2069 to 0x0869*/
  0x047D,/* 6 converted 0x00FD to 0x047D*/
  0x0007,/* 7 converted 0x0007 to 0x0007*/
  0x047F,/* 8 converted 0x00FF to 0x047F*/
  0x046F,/* 9 converted 0x00EF to 0x046F*/
  0x1100,/* : converted 0x1200 to 0x1100*/
  0x2100,/* ; converted 0x0A00 to 0x2100*/
  0x0A40,/* < converted 0x2440 to 0x0A40*/
  0x0448,/* = converted 0x00C8 to 0x0448*/
  0x2480,/* > converted 0x0980 to 0x2480*/
  0x1403,/* ? converted 0x5083 to 0x1403*/
  0x053B,/* @ converted 0x02BB to 0x053B*/
  0x0477,/* A converted 0x00F7 to 0x0477*/
  0x150F,/* B converted 0x128F to 0x150F*/
  0x0039,/* C converted 0x0039 to 0x0039*/
  0x110F,/* D converted 0x120F to 0x110F*/
  0x0079,/* E converted 0x0079 to 0x0079*/
  0x0071,/* F converted 0x0071 to 0x0071*/
  0x043D,/* G converted 0x00BD to 0x043D*/
  0x0476,/* H converted 0x00F6 to 0x0476*/
  0x1109,/* I converted 0x1209 to 0x1109*/
  0x001E,/* J converted 0x001E to 0x001E*/
  0x0A70,/* K converted 0x2470 to 0x0A70*/
  0x0038,/* L converted 0x0038 to 0x0038*/
  0x02B6,/* M converted 0x0536 to 0x02B6*/
  0x08B6,/* N converted 0x2136 to 0x08B6*/
  0x003F,/* O converted 0x003F to 0x003F*/
  0x0473,/* P converted 0x00F3 to 0x0473*/
  0x083F,/* Q converted 0x203F to 0x083F*/
  0x0C73,/* R converted 0x20F3 to 0x0C73*/
  0x046D,/* S converted 0x00ED to 0x046D*/
  0x1101,/* T converted 0x1201 to 0x1101*/
  0x003E,/* U converted 0x003E to 0x003E*/
  0x2230,/* V converted 0x0C30 to 0x2230*/
  0x2836,/* W converted 0x2836 to 0x2836*/
  0x2A80,/* X converted 0x2D00 to 0x2A80*/
  0x046E,/* Y converted 0x00EE to 0x046E*/
  0x2209,/* Z converted 0x0C09 to 0x2209*/
  0x0039,/* [ converted 0x0039 to 0x0039*/
  0x0880,/* \ converted 0x2100 to 0x0880*/
  0x000F,/* ] converted 0x000F to 0x000F*/
  0x2800,/* ^ converted 0x2800 to 0x2800*/
  0x0008,/* _ converted 0x0008 to 0x0008*/
  0x0080,/* ` converted 0x0100 to 0x0080*/
  0x1058,/* a converted 0x1058 to 0x1058*/
  0x0878,/* b converted 0x2078 to 0x0878*/
  0x0458,/* c converted 0x00D8 to 0x0458*/
  0x240E,/* d converted 0x088E to 0x240E*/
  0x2058,/* e converted 0x0858 to 0x2058*/
  0x1640,/* f converted 0x14C0 to 0x1640*/
  0x060E,/* g converted 0x048E to 0x060E*/
  0x1070,/* h converted 0x1070 to 0x1070*/
  0x1000,/* i converted 0x1000 to 0x1000*/
  0x2110,/* j converted 0x0A10 to 0x2110*/
  0x1B00,/* k converted 0x3600 to 0x1B00*/
  0x0030,/* l converted 0x0030 to 0x0030*/
  0x1454,/* m converted 0x10D4 to 0x1454*/
  0x1050,/* n converted 0x1050 to 0x1050*/
  0x045C,/* o converted 0x00DC to 0x045C*/
  0x00F0,/* p converted 0x0170 to 0x00F0*/
  0x0606,/* q converted 0x0486 to 0x0606*/
  0x0050,/* r converted 0x0050 to 0x0050*/
  0x0C08,/* s converted 0x2088 to 0x0C08*/
  0x0078,/* t converted 0x0078 to 0x0078*/
  0x001C,/* u converted 0x001C to 0x001C*/
  0x2010,/* v converted 0x0810 to 0x2010*/
  0x2814,/* w converted 0x2814 to 0x2814*/
  0x2A80,/* x converted 0x2D00 to 0x2A80*/
  0x050E,/* y converted 0x028E to 0x050E*/
  0x2048,/* z converted 0x0848 to 0x2048*/
  0x20C9,/* { converted 0x0949 to 0x20C9*/
  0x1100,/* | converted 0x1200 to 0x1100*/
  0x0E09,/* } converted 0x2489 to 0x0E09*/
  0x2640,/* ~ converted 0x0CC0 to 0x2640*/
  0x0000 /*  converted 0x0000 to 0x0000*/
};

#endif

  
#endif


#define RPU_OS_H
#endif
