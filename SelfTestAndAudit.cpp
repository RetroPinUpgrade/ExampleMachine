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

#include <Arduino.h>
#include "RPU_Config.h"
#include "RPU.h"
#include "SelfTestAndAudit.h"

#define MACHINE_STATE_ATTRACT         0

unsigned long LastSolTestTime = 0; 
unsigned long LastSelfTestChange = 0;
unsigned long SavedValue = 0;
unsigned long ResetHold = 0;
unsigned long NextSpeedyValueChange = 0;
unsigned long NumSpeedyChanges = 0;
unsigned long LastResetPress = 0;
byte CurValue = 0;
byte CurSound = 0x01;
byte SoundPlaying = 0;
byte SoundToPlay = 0;
boolean SolenoidCycle = true;

#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE
boolean CPCSelectionsHaveBeenRead = false;
#define NUM_CPC_PAIRS 9
byte CPCPairs[NUM_CPC_PAIRS][2] = {
  {1, 5},
  {1, 4},
  {1, 3},
  {1, 2},
  {1, 1},
  {2, 3},
  {2, 1},
  {3, 1},
  {4, 1}
};
byte CPCSelection[3];


byte GetCPCSelection(byte chuteNumber) {
  if (chuteNumber>2) return 0xFF;

  if (CPCSelectionsHaveBeenRead==false) {
    CPCSelection[0] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE);
    if (CPCSelection[0]>=NUM_CPC_PAIRS) {
      CPCSelection[0] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE, 4);
    }
    CPCSelection[1] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE);  
    if (CPCSelection[1]>=NUM_CPC_PAIRS) {
      CPCSelection[1] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE, 4);
    }
    CPCSelection[2] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE);  
    if (CPCSelection[2]>=NUM_CPC_PAIRS) {
      CPCSelection[2] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE, 4);
    }
    CPCSelectionsHaveBeenRead = true;
  }
  
  return CPCSelection[chuteNumber];
}


byte GetCPCCoins(byte cpcSelection) {
  if (cpcSelection>=NUM_CPC_PAIRS) return 1;
  return CPCPairs[cpcSelection][0];
}


byte GetCPCCredits(byte cpcSelection) {
  if (cpcSelection>=NUM_CPC_PAIRS) return 1;
  return CPCPairs[cpcSelection][1];
}
#endif

#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
#define TOTAL_DISPLAY_DIGITS 34
#else
#define TOTAL_DISPLAY_DIGITS 35
#endif
#else
#define TOTAL_DISPLAY_DIGITS 30
#endif

int RunBaseSelfTest(int curState, boolean curStateChanged, unsigned long CurrentTime, byte resetSwitch, byte slamSwitch) {
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  int returnState = curState;
  boolean resetDoubleClick = false;
  unsigned short savedScoreStartByte = 0;
  unsigned short auditNumStartByte = 0;
#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE  
  unsigned short cpcSelectorStartByte = 0;
#endif

  if (curSwitch==resetSwitch) {
    ResetHold = CurrentTime;
    if ((CurrentTime-LastResetPress)<400) {
      resetDoubleClick = true;
      curSwitch = SWITCH_STACK_EMPTY;
    }
    LastResetPress = CurrentTime;
    SoundToPlay += 1;
    if (SoundToPlay>31) SoundToPlay = 0;
  }

  if (ResetHold!=0 && !RPU_ReadSingleSwitchState(resetSwitch)) {
    ResetHold = 0;
    NextSpeedyValueChange = 0;
  }

  boolean resetBeingHeld = false;
  if (ResetHold!=0 && (CurrentTime-ResetHold)>1300) {
    resetBeingHeld = true;
    if (NextSpeedyValueChange==0) {
      NextSpeedyValueChange = CurrentTime;
      NumSpeedyChanges = 0;
    }
  }

  if (slamSwitch!=0xFF && curSwitch==slamSwitch) {
    returnState = MACHINE_STATE_ATTRACT;
  }
  
  if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime-LastSelfTestChange)>250) {
    if (RPU_GetUpDownSwitchState()) returnState -= 1;
    else returnState += 1;
//    if (returnState==MACHINE_STATE_TEST_DONE) returnState = MACHINE_STATE_ATTRACT;
    LastSelfTestChange = CurrentTime;
  }

  if (curStateChanged) {
    RPU_SetCoinLockout(false);
    
    for (int count=0; count<4; count++) {
      RPU_SetDisplay(count, 0);
      RPU_SetDisplayBlank(count, 0x00);        
    }

#if (RPU_MPU_ARCHITECTURE<10)
    if (curState<=MACHINE_STATE_TEST_SCORE_LEVEL_1) {
      RPU_SetDisplayCredits(0, false);
      RPU_SetDisplayBallInPlay(MACHINE_STATE_TEST_SOUNDS-curState);
    } else {
      RPU_SetDisplayCredits(0 - curState, true);
      RPU_SetDisplayBallInPlay(0, false);      
    }
#else
    if (curState<=MACHINE_STATE_TEST_HISCR) {
      RPU_SetDisplayCredits(0, false);
      RPU_SetDisplayBallInPlay(MACHINE_STATE_TEST_BOOT-curState, true);
    } else {
      RPU_SetDisplayCredits(curState - MACHINE_STATE_TEST_BOOT, true);
      RPU_SetDisplayBallInPlay(0, false);      
    }
#endif      
  }

  if (curState==MACHINE_STATE_TEST_LAMPS) {
    if (curStateChanged) {
      RPU_DisableSolenoidStack();        
      RPU_SetDisableFlippers(true);
      RPU_TurnOffAllLamps();
      for (int count=0; count<RPU_MAX_LAMPS; count++) {
        RPU_SetLampState(count, 1, 0, 500);
      }
      CurValue = 99;
      RPU_SetDisplay(0, CurValue, true);  
    }
    if (curSwitch==resetSwitch || resetDoubleClick) {
      if (RPU_GetUpDownSwitchState()) {
        CurValue += 1;
        if (CurValue==RPU_MAX_LAMPS) CurValue = 99;
        else if (CurValue>99) CurValue = 0;    
      } else {
        if (CurValue>0) CurValue -= 1;
        else CurValue = 99;
        if (CurValue==98) CurValue = RPU_MAX_LAMPS - 1;
      }
      if (CurValue==99) {
        for (int count=0; count<RPU_MAX_LAMPS; count++) {
          RPU_SetLampState(count, 1, 0, 500);
        }
      } else {
        RPU_TurnOffAllLamps();
        RPU_SetLampState(CurValue, 1);
      }      
      RPU_SetDisplay(0, CurValue, true);  
    }    
  } else if (curState==MACHINE_STATE_TEST_DISPLAYS) {
    if (curStateChanged) {
      RPU_TurnOffAllLamps();
      for (int count=0; count<4; count++) {
        RPU_SetDisplayBlank(count, RPU_OS_ALL_DIGITS_MASK);        
      }
      CurValue = 0;
    }
    if (curSwitch==resetSwitch || resetDoubleClick) {
      if (RPU_GetUpDownSwitchState()) {
        CurValue += 1;
        if (CurValue>TOTAL_DISPLAY_DIGITS) {
          for (int count=0; count<4; count++) {
            RPU_SetDisplayBlank(count, RPU_OS_ALL_DIGITS_MASK);        
          }
          CurValue = 0;
        }
      } else {
        if (CurValue>0) CurValue -= 1;
        else CurValue = TOTAL_DISPLAY_DIGITS;
      }
    }    
    RPU_CycleAllDisplays(CurrentTime, CurValue);
  } else if (curState==MACHINE_STATE_TEST_SOLENOIDS) {
    if (curStateChanged) {
      RPU_TurnOffAllLamps();
      LastSolTestTime = CurrentTime;
      RPU_EnableSolenoidStack(); 
      RPU_SetDisableFlippers(false);
      //RPU_SetDisplayBlank(4, 0);
      SolenoidCycle = true;
      SavedValue = 0;
      RPU_PushToSolenoidStack(SavedValue, 10);
    } 
    if (curSwitch==resetSwitch || resetDoubleClick) {
      SolenoidCycle = (SolenoidCycle) ? false : true;
    }

    if ((CurrentTime-LastSolTestTime)>1000) {
      if (SolenoidCycle) {
        SavedValue += 1;
#if (RPU_MPU_ARCHITECTURE<10)
        if (SavedValue>14) SavedValue = 0;
#else        
        if (SavedValue>21) SavedValue = 0;
#endif        
      }
      RPU_PushToSolenoidStack(SavedValue, 10);
      RPU_SetDisplay(0, SavedValue, true);
      LastSolTestTime = CurrentTime;
    }
    
  } else if (curState==MACHINE_STATE_TEST_SWITCHES) {
    if (curStateChanged) {
      RPU_TurnOffAllLamps();
      RPU_DisableSolenoidStack(); 
      RPU_SetDisableFlippers(true);
    }

    byte displayOutput = 0;
    for (byte switchCount=0; switchCount<64 && displayOutput<4; switchCount++) {
      if (RPU_ReadSingleSwitchState(switchCount)) {
        RPU_SetDisplay(displayOutput, switchCount, true);
        displayOutput += 1;
      }
    }

    if (displayOutput<4) {
      for (int count=displayOutput; count<4; count++) {
        RPU_SetDisplayBlank(count, 0x00);
      }
    }

  } else if (curState==MACHINE_STATE_TEST_SOUNDS) {
#ifdef RPU_OS_USE_SB100    
    byte soundToPlay = 0x01 << (((CurrentTime-LastSelfTestChange)/750)%8);
    if (SoundPlaying!=soundToPlay) {
      RPU_PlaySB100(soundToPlay);
      SoundPlaying = soundToPlay;
      RPU_SetDisplay(0, (unsigned long)soundToPlay, true);
      LastSolTestTime = CurrentTime; // Time the sound started to play
    }
    // If the sound play call was more than 300ms ago, turn it off
//    if ((CurrentTime-LastSolTestTime)>300) RPU_PlaySB100(128);
#elif defined (RPU_OS_USE_S_AND_T)
    byte soundToPlay = ((CurrentTime-LastSelfTestChange)/2000)%256;
    if (SoundPlaying!=soundToPlay) {
      RPU_PlaySoundSAndT(soundToPlay);
      SoundPlaying = soundToPlay;
      RPU_SetDisplay(0, (unsigned long)soundToPlay, true);
      LastSolTestTime = CurrentTime; // Time the sound started to play
    }
#elif defined (RPU_OS_USE_DASH51) 
    byte soundToPlay = ((CurrentTime-LastSelfTestChange)/2000)%32;
    if (SoundPlaying!=soundToPlay) {
      if (soundToPlay==17) soundToPlay = 0;
      RPU_PlaySoundDash51(soundToPlay);
      SoundPlaying = soundToPlay;
      RPU_SetDisplay(0, (unsigned long)soundToPlay, true);
      LastSolTestTime = CurrentTime; // Time the sound started to play
    }
#elif defined (RPU_OS_USE_WTYPE_1_SOUND)
    byte soundToPlay = (((CurrentTime-LastSelfTestChange)/2000)%31)+1;
    if (SoundPlaying!=soundToPlay) {
      RPU_PushToSoundStack(soundToPlay*256, 8);
      SoundPlaying = soundToPlay;
      RPU_SetDisplay(0, (unsigned long)soundToPlay, true);
      LastSolTestTime = CurrentTime; // Time the sound started to play
    }
#elif defined (RPU_OS_USE_WTYPE_2_SOUND) 
//    byte soundToPlay = (((CurrentTime-LastSelfTestChange)/1000)%32);
    if (SoundPlaying!=SoundToPlay) {
      RPU_PushToSoundStack(SoundToPlay, 8);
      SoundPlaying = SoundToPlay  ;
      RPU_SetDisplay(0, (unsigned long)SoundToPlay, true);
      LastSolTestTime = CurrentTime; // Time the sound started to play
    }
#endif
  } else if (curState==MACHINE_STATE_TEST_BOOT) {
    if (curStateChanged) {
      for (int count=0; count<4; count++) {
        RPU_SetDisplay(count, 8007, true);
      }
    }
    if (curSwitch==resetSwitch || resetDoubleClick) {
      returnState = MACHINE_STATE_ATTRACT;
    }
    for (int count=0; count<4; count++) {
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
      RPU_SetDisplayBlank(count, ((CurrentTime/500)%2)?0x78:0x00);
#else      
      RPU_SetDisplayBlank(count, ((CurrentTime/500)%2)?0x3C:0x00);
#endif      
    }
  } else if (curState==MACHINE_STATE_TEST_SCORE_LEVEL_1) {
#ifdef RPU_OS_USE_SB100    
    if (curStateChanged) RPU_PlaySB100(0);
#endif
    savedScoreStartByte = RPU_AWARD_SCORE_1_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_SCORE_LEVEL_2) {
    savedScoreStartByte = RPU_AWARD_SCORE_2_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_SCORE_LEVEL_3) {
    savedScoreStartByte = RPU_AWARD_SCORE_3_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_HISCR) {
    savedScoreStartByte = RPU_HIGHSCORE_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_CREDITS) {
    if (curStateChanged) {
      SavedValue = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
      RPU_SetDisplay(0, SavedValue, true);
    }
    if (curSwitch==resetSwitch || resetDoubleClick) {
      if (RPU_GetUpDownSwitchState()) {
        SavedValue += 1;
        if (SavedValue>99) SavedValue = 0;
      } else {
        if (SavedValue>0) SavedValue -= 1;
        else SavedValue = 99;
      }
      RPU_SetDisplay(0, SavedValue, true);
      RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, SavedValue & 0x000000FF);
    }
  } else if (curState==MACHINE_STATE_TEST_TOTAL_PLAYS) {
    auditNumStartByte = RPU_TOTAL_PLAYS_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_TOTAL_REPLAYS) {
    auditNumStartByte = RPU_TOTAL_REPLAYS_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_HISCR_BEAT) {
    auditNumStartByte = RPU_TOTAL_HISCORE_BEATEN_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_CHUTE_2_COINS) {
    auditNumStartByte = RPU_CHUTE_2_COINS_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_CHUTE_1_COINS) {
    auditNumStartByte = RPU_CHUTE_1_COINS_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_CHUTE_3_COINS) {
    auditNumStartByte = RPU_CHUTE_3_COINS_START_BYTE;
#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE      
  } else if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_1) {
    cpcSelectorStartByte = RPU_CPC_CHUTE_1_SELECTION_BYTE;
  } else if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_2) {
    cpcSelectorStartByte = RPU_CPC_CHUTE_2_SELECTION_BYTE;
  } else if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_3) {
    cpcSelectorStartByte = RPU_CPC_CHUTE_3_SELECTION_BYTE;
#endif    
  }

  if (savedScoreStartByte) {
    if (curStateChanged) {
      SavedValue = RPU_ReadULFromEEProm(savedScoreStartByte);
      RPU_SetDisplay(0, SavedValue, true);  
    }

    if (curSwitch==resetSwitch) {
      if (RPU_GetUpDownSwitchState()) {
        SavedValue += 1000;
      } else {
        if (SavedValue>1000) SavedValue -= 1000;
        else SavedValue = 0;
      }
      RPU_SetDisplay(0, SavedValue, true);  
      RPU_WriteULToEEProm(savedScoreStartByte, SavedValue);
    }

    if (resetBeingHeld && (CurrentTime>=NextSpeedyValueChange)) {
      if (RPU_GetUpDownSwitchState()) {
        SavedValue += 1000;
      } else {
        if (SavedValue>1000) SavedValue -= 1000;
        else SavedValue = 0;
      }
      RPU_SetDisplay(0, SavedValue, true);  
      if (NumSpeedyChanges<6) NextSpeedyValueChange = CurrentTime + 400;
      else if (NumSpeedyChanges<50) NextSpeedyValueChange = CurrentTime + 50;
      else NextSpeedyValueChange = CurrentTime + 10;
      NumSpeedyChanges += 1;
    }

    if (!resetBeingHeld && NumSpeedyChanges>0) {
      RPU_WriteULToEEProm(savedScoreStartByte, SavedValue);
      NumSpeedyChanges = 0;
    }
    
    if (resetDoubleClick) {
      SavedValue = 0;
      RPU_SetDisplay(0, SavedValue, true);  
      RPU_WriteULToEEProm(savedScoreStartByte, SavedValue);
    }
  }

  if (auditNumStartByte) {
    if (curStateChanged) {
      SavedValue = RPU_ReadULFromEEProm(auditNumStartByte);
      RPU_SetDisplay(0, SavedValue, true);
    }

    if (resetDoubleClick) {
      SavedValue = 0;
      RPU_SetDisplay(0, SavedValue, true);  
      RPU_WriteULToEEProm(auditNumStartByte, SavedValue);
    }
    
  }

#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE  
  if (cpcSelectorStartByte) {
    if (curStateChanged) {
      SavedValue = RPU_ReadByteFromEEProm(cpcSelectorStartByte);
      if (SavedValue>NUM_CPC_PAIRS) SavedValue = 4;
      RPU_SetDisplay(0, CPCPairs[SavedValue][0], true);
      RPU_SetDisplay(1, CPCPairs[SavedValue][1], true);
    }

    if (curSwitch==resetSwitch) {
      byte lastValue = (byte)SavedValue;
      if (RPU_GetUpDownSwitchState()) {
        SavedValue += 1;
        if (SavedValue>=NUM_CPC_PAIRS) SavedValue = 0;
      } else {
        if (SavedValue>0) SavedValue -= 1;
      }
      RPU_SetDisplay(0, CPCPairs[SavedValue][0], true);
      RPU_SetDisplay(1, CPCPairs[SavedValue][1], true);
      if (lastValue!=SavedValue) {
        RPU_WriteByteToEEProm(cpcSelectorStartByte, (byte)SavedValue);
        if (cpcSelectorStartByte==RPU_CPC_CHUTE_1_SELECTION_BYTE) CPCSelection[0] = (byte)SavedValue;
        else if (cpcSelectorStartByte==RPU_CPC_CHUTE_2_SELECTION_BYTE) CPCSelection[1] = (byte)SavedValue;
        else if (cpcSelectorStartByte==RPU_CPC_CHUTE_3_SELECTION_BYTE) CPCSelection[2] = (byte)SavedValue;
      }
    }
  }
#endif  
  
  return returnState;
}

unsigned long GetLastSelfTestChangedTime() {
  return LastSelfTestChange;
}


void SetLastSelfTestChangedTime(unsigned long setSelfTestChange) {
  LastSelfTestChange = setSelfTestChange;
}
