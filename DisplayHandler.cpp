////////////////////////////////////////////////////////////////////////////
//
//  Display Management functions
//
////////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include "RPU_config.h"
#include "RPU.h"
#include "DisplayHandler.h"

#ifndef RPU_NUMBER_OF_PLAYER_DISPLAYS
#define RPU_NUMBER_OF_PLAYER_DISPLAYS       4
#endif

#ifdef RPU_DMD_DISPLAYS
#include <Wire.h>
#define DISPLAY_NUM_DIGITS                  RPU_OS_NUM_DIGITS
#define DISPLAY_NUMBER_OF_PLAYER_DISPLAYS   RPU_NUMBER_OF_PLAYER_DISPLAYS
#else
#define DISPLAY_NUM_DIGITS                  RPU_OS_NUM_DIGITS
#define DISPLAY_NUMBER_OF_PLAYER_DISPLAYS   RPU_NUMBER_OF_PLAYER_DISPLAYS
#endif


extern unsigned long CurrentTime;
extern unsigned long CurrentScores[RPU_NUMBER_OF_PLAYERS_ALLOWED];
extern unsigned long BallFirstSwitchHitTime;
extern byte CurrentPlayer;
extern byte CurrentAchievements[RPU_NUMBER_OF_PLAYERS_ALLOWED];
extern byte CurrentNumPlayers;


unsigned long ScoreAdditionAnimation;
unsigned long ScoreAdditionAnimationStartTime;
unsigned long LastRemainingAnimatedScoreShown;
unsigned long PlayScoreAnimationTick = 2500;
byte JackpotScoreAnimationType = DISPLAY_JACKPOT_ANIMATION_OFF;
byte PlayTickLevel = 0;
byte ScoreAdditionLastPhase;


unsigned long LastTimeScoreChanged = 0;
unsigned long LastFlashOrDash = 0;
unsigned long ScoreOverrideValue[4] = {0, 0, 0, 0};
byte LastAnimationSeed[4] = {0, 0, 0, 0};
byte AnimationStartSeed[4] = {0, 0, 0, 0};
byte ScoreOverrideStatus = 0;
byte ScoreAnimation[4] = {0, 0, 0, 0};
byte AnimationDisplayOrder[4] = {0, 1, 2, 3};
byte OverrideMask[4] = {0xFF, 0xFF, 0xFF, 0xFF};
byte LastScrollPhase[RPU_NUMBER_OF_PLAYER_DISPLAYS] = {0};

boolean DisplayAchievements = false;





byte Display_MagnitudeOfScore(unsigned long score) {
  if (score == 0) return 0;

  byte retval = 0;
  while (score > 0) {
    score = score / 10;
    retval += 1;
  }
  return retval;
}

void Display_EnableAchievements(boolean achievementsShown) {
  DisplayAchievements = achievementsShown;
}


void Display_ResetDisplayTrackingVariables() {
  ScoreAdditionAnimation = 0;
  ScoreAdditionAnimationStartTime = 0;
  LastRemainingAnimatedScoreShown = 0;
  ScoreAdditionLastPhase = 0xFF;
  PlayTickLevel = 0;
}


void Display_SetLastTimeScoreChanged(unsigned long scoreChangedTime) {
  LastTimeScoreChanged = scoreChangedTime;
}

unsigned long Display_GetLastTimeScoreChanged() {
  return LastTimeScoreChanged;
}


void Display_OverrideScoreDisplay(byte displayNum, unsigned long value, byte animationType, byte overrideMask) {
  if (displayNum > (DISPLAY_NUMBER_OF_PLAYER_DISPLAYS-1)) return;

  ScoreOverrideStatus |= (0x01 << displayNum);
  ScoreAnimation[displayNum] = animationType;
  ScoreOverrideValue[displayNum] = value;
  LastAnimationSeed[displayNum] = 255;
  OverrideMask[displayNum] = overrideMask;
}


void Display_ClearOverride(byte displayNum) {
  if (displayNum==0xFF) {
    ScoreOverrideStatus = 0;
  } else {
    ScoreOverrideStatus &= ~(0x01 << displayNum);
  }
}

byte GetDisplayMask(byte numDigits) {
  byte displayMask = 0;
  for (byte digitCount = 0; digitCount < numDigits; digitCount++) {
#if (DISPLAY_NUM_DIGITS==7)
    displayMask |= (0x40 >> digitCount);
#else
    displayMask |= (0x20 >> digitCount);
#endif
  }
  return displayMask;
}


void Display_SetAnimationDisplayOrder(byte disp0, byte disp1, byte disp2, byte disp3) {
  AnimationDisplayOrder[0] = disp0;
  AnimationDisplayOrder[1] = disp1;
  AnimationDisplayOrder[2] = disp2;
  AnimationDisplayOrder[3] = disp3;
}





void ShowAnimatedValue(byte displayNum, unsigned long displayScore, byte animationType) {
  byte overrideAnimationSeed;
  byte displayMask = RPU_OS_ALL_DIGITS_MASK;

  byte numDigits = Display_MagnitudeOfScore(displayScore);
  if (numDigits == 0) numDigits = 1;
  if (numDigits < (DISPLAY_NUM_DIGITS - 1) && animationType == DISPLAY_OVERRIDE_ANIMATION_BOUNCE) {
    // This score is going to be animated (back and forth)
    overrideAnimationSeed = (CurrentTime / 250) % (2 * DISPLAY_NUM_DIGITS - 2 * numDigits);
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      byte shiftDigits = (overrideAnimationSeed);
      if (shiftDigits >= ((DISPLAY_NUM_DIGITS + 1) - numDigits)) shiftDigits = (DISPLAY_NUM_DIGITS - numDigits) * 2 - shiftDigits;
      byte digitCount;
      displayMask = GetDisplayMask(numDigits);
      for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
        displayScore *= 10;
        displayMask = displayMask >> 1;
      }
      //RPU_SetDisplayBlank(displayNum, 0x00);
      RPU_SetDisplay(displayNum, displayScore, false);
      RPU_SetDisplayBlank(displayNum, displayMask);
    }
  } else if (animationType == DISPLAY_OVERRIDE_ANIMATION_FLUTTER) {
    overrideAnimationSeed = CurrentTime / 50;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      displayMask = GetDisplayMask(numDigits);
      if (overrideAnimationSeed % 2) {
        displayMask &= 0x55;
      } else {
        displayMask &= 0xAA;
      }
      RPU_SetDisplay(displayNum, displayScore, false);
      RPU_SetDisplayBlank(displayNum, displayMask);
    }
  } else if (animationType == DISPLAY_OVERRIDE_ANIMATION_FLYBY) {
    overrideAnimationSeed = (CurrentTime / 75) % 256;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      if (LastAnimationSeed[displayNum] == 255) {
        AnimationStartSeed[displayNum] = overrideAnimationSeed;
      }
      LastAnimationSeed[displayNum] = overrideAnimationSeed;

      byte realAnimationSeed = overrideAnimationSeed - AnimationStartSeed[displayNum];
      if (overrideAnimationSeed < AnimationStartSeed[displayNum]) realAnimationSeed = (255 - AnimationStartSeed[displayNum]) + overrideAnimationSeed;

      if (realAnimationSeed > 34) {
        RPU_SetDisplayBlank(displayNum, 0x00);
        ScoreOverrideStatus &= ~(0x01 << displayNum);
      } else {
        int shiftDigits = (-6 * ((int)AnimationDisplayOrder[displayNum] + 1)) + realAnimationSeed;
        displayMask = GetDisplayMask(numDigits);
        if (shiftDigits < 0) {
          shiftDigits = 0 - shiftDigits;
          byte digitCount;
          for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
            displayScore /= 10;
            displayMask = displayMask << 1;
          }
        } else if (shiftDigits > 0) {
          byte digitCount;
          for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
            displayScore *= 10;
            displayMask = displayMask >> 1;
          }
        }
        RPU_SetDisplay(displayNum, displayScore, false);
        RPU_SetDisplayBlank(displayNum, displayMask);
      }
    }
  } else if ( animationType == DISPLAY_OVERRIDE_ANIMATION_CENTER ||
              animationType == DISPLAY_OVERRIDE_CENTER_FLASH_SLOW || 
              animationType == DISPLAY_OVERRIDE_CENTER_FLASH_FAST ) {
    overrideAnimationSeed = CurrentTime / 175;
    if (animationType==DISPLAY_OVERRIDE_CENTER_FLASH_FAST) overrideAnimationSeed = CurrentTime / 75;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      byte shiftDigits = (DISPLAY_NUM_DIGITS - numDigits) / 2;
      byte digitCount;
      displayMask = GetDisplayMask(numDigits);
      for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
        displayScore *= 10;
        displayMask = displayMask >> 1;
      }
      //RPU_SetDisplayBlank(displayNum, 0x00);
      RPU_SetDisplay(displayNum, displayScore, false);

      if (animationType == DISPLAY_OVERRIDE_ANIMATION_CENTER) {
        RPU_SetDisplayBlank(displayNum, displayMask);
      } else {
        boolean flashOn = (overrideAnimationSeed%2) ? true : false;
        if (flashOn) RPU_SetDisplayBlank(displayNum, displayMask);
        else RPU_SetDisplayBlank(displayNum, 0x00);
      }
    }
  } else if (numDigits==1 && animationType == DISPLAY_OVERRIDE_SYMMETRIC_BOUNCE) {
    // Timing varies based on display
    overrideAnimationSeed = CurrentTime / (100 + (displayNum%2)*38 + (displayNum/2)*20);

    // If the animation frame has changed since last time
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      LastAnimationSeed[displayNum] = overrideAnimationSeed;

      byte numSteps = DISPLAY_NUM_DIGITS - 2 + (DISPLAY_NUM_DIGITS%2);
      
      byte shiftDigits = (overrideAnimationSeed)%numSteps;
      if (shiftDigits > (numSteps/2)) shiftDigits = numSteps - shiftDigits;
        byte digitCount;
      displayMask = GetDisplayMask(numDigits);
      for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
        displayScore *= 10;
        displayMask = displayMask >> 1;
      }
      unsigned long newScore = displayScore;
      byte newMask = displayMask;
      // Now we mirror it
      if (shiftDigits<(DISPLAY_NUM_DIGITS/2)) {
        for (digitCount = 0; digitCount < (DISPLAY_NUM_DIGITS-1-(shiftDigits*2)); digitCount++) {
          displayScore *= 10;
          displayMask = displayMask >> 1;
        }
        newScore += displayScore;
        newMask |= displayMask;
      }
      RPU_SetDisplay(displayNum, newScore, false);
      RPU_SetDisplayBlank(displayNum, newMask);
    }
  } else {
    if (OverrideMask[displayNum]==0xFF) {
      RPU_SetDisplay(displayNum, displayScore, true, 1);
    } else {
      RPU_SetDisplayBlank(displayNum, OverrideMask[displayNum]);
      RPU_SetDisplay(displayNum, displayScore, false, 1);
    }
  }

}


void ShowPlayerScore(unsigned long scoreToShow, byte displayToUpdate, boolean flashCurrent = false, byte dashCurrent = false) {

  byte displayMask = RPU_OS_ALL_DIGITS_MASK;
  unsigned long displayScore = 0;
  byte scrollPhaseChanged = false;

  byte scrollPhase = ((CurrentTime - LastTimeScoreChanged) / 125) % 16;
  if (scrollPhase != LastScrollPhase[displayToUpdate]) {
    LastScrollPhase[displayToUpdate] = scrollPhase;
    scrollPhaseChanged = true;
  }

  boolean showingCurrentAchievement = false;
  // No override, update scores designated by displayToUpdate
  displayScore = scoreToShow;
  if (DisplayAchievements) {
    if (CurrentAchievements[displayToUpdate]) {
      displayScore -= (displayScore%10);
      displayScore += (CurrentAchievements[displayToUpdate] % 10);
      showingCurrentAchievement = true;
    }
  }

  if (displayScore > RPU_OS_MAX_DISPLAY_SCORE) {
    // Score needs to be scrolled
    if ((CurrentTime - LastTimeScoreChanged) < 2000) {
      // show score for four seconds after change
      RPU_SetDisplay(displayToUpdate, displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1), false);
      byte blank = RPU_OS_ALL_DIGITS_MASK;
      if (showingCurrentAchievement && (CurrentTime / 200) % 2) {
        blank &= ~(0x01 << (DISPLAY_NUM_DIGITS - 1));
      }
      RPU_SetDisplayBlank(displayToUpdate, blank);
    } else {
      // Scores are scrolled 10 digits and then we wait for 6
      if (scrollPhase < 11 && scrollPhaseChanged) {
        byte numDigits = Display_MagnitudeOfScore(displayScore);

        // Figure out top part of score
        unsigned long tempScore = displayScore;
        if (scrollPhase < DISPLAY_NUM_DIGITS) {
          displayMask = RPU_OS_ALL_DIGITS_MASK;
          if (showingCurrentAchievement && (CurrentTime / 200) % 2) {
            displayMask &= ~(0x01 << (DISPLAY_NUM_DIGITS - 1));
          }          
          for (byte scrollCount = 0; scrollCount < scrollPhase; scrollCount++) {
            displayScore = (displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1)) * 10;
            displayMask = displayMask >> 1;
          }
        } else {
          displayScore = 0;
          displayMask = 0x00;
        }

        // Add in lower part of score
        if ((numDigits + scrollPhase) > 10) {
          byte numDigitsNeeded = (numDigits + scrollPhase) - 10;
          for (byte scrollCount = 0; scrollCount < (numDigits - numDigitsNeeded); scrollCount++) {
            tempScore /= 10;
          }
          displayMask |= GetDisplayMask(Display_MagnitudeOfScore(tempScore));
          displayScore += tempScore;
        }
        RPU_SetDisplayBlank(displayToUpdate, displayMask);
        RPU_SetDisplay(displayToUpdate, displayScore);
      }
    }
  } else {
    if (flashCurrent) {
      unsigned long flashSeed = CurrentTime / 250;
      if (flashSeed != LastFlashOrDash) {
        LastFlashOrDash = flashSeed;
        if (((CurrentTime / 250) % 2) == 0) RPU_SetDisplayBlank(displayToUpdate, 0x00);
        else RPU_SetDisplay(displayToUpdate, displayScore, true, 2, true);
      }
    } else if (dashCurrent) {
      if (dashCurrent==DISPLAY_DASH_ROLLING_BLANK) {
        unsigned long dashSeed = CurrentTime / 50;
        if (dashSeed != LastFlashOrDash) {
          LastFlashOrDash = dashSeed;
          byte dashPhase = (CurrentTime / 60) % (2 * DISPLAY_NUM_DIGITS * 3);
          byte numDigits = Display_MagnitudeOfScore(displayScore);
          if (dashPhase < (2 * DISPLAY_NUM_DIGITS)) {
            displayMask = GetDisplayMask((numDigits == 0) ? 2 : numDigits);
            if (dashPhase < (DISPLAY_NUM_DIGITS + 1)) {
              for (byte maskCount = 0; maskCount < dashPhase; maskCount++) {
                displayMask &= ~(0x01 << maskCount);
              }
            } else {
              for (byte maskCount = (2 * DISPLAY_NUM_DIGITS); maskCount > dashPhase; maskCount--) {                    
                byte firstDigit;
                firstDigit = (0x20) << (DISPLAY_NUM_DIGITS - 6);
                displayMask &= ~(firstDigit >> (maskCount - dashPhase - 1));
              }
            }
            RPU_SetDisplay(displayToUpdate, displayScore);
            RPU_SetDisplayBlank(displayToUpdate, displayMask);
          } else {
            RPU_SetDisplay(displayToUpdate, displayScore, true, 2, true);
          }
        }
      } else if (dashCurrent==DISPLAY_DASH_INTERMITTENT_FLASH) {
        unsigned long dashSeed = (CurrentTime / 250)%4;
        if (dashSeed != LastFlashOrDash) {
          LastFlashOrDash = dashSeed;
          if (dashSeed) {
            RPU_SetDisplay(displayToUpdate, displayScore, true, 2, true);
          } else {
            RPU_SetDisplayBlank(displayToUpdate, 0x00);
          }
        }
      }
    } else {
      byte blank;
      blank = RPU_SetDisplay(displayToUpdate, displayScore, false, 2, true);
      if (showingCurrentAchievement && (CurrentTime / 200) % 2) {
        blank &= ~(0x01 << (DISPLAY_NUM_DIGITS - 1));
      }
      RPU_SetDisplayBlank(displayToUpdate, blank);
    }
  }

}



void Display_StartScoreAnimation(unsigned long scoreToAnimate, boolean playTick, byte animationType) {
  if (ScoreAdditionAnimation != 0) {
    CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
    ScoreAdditionAnimation = 0;
  }
  ScoreAdditionAnimation += scoreToAnimate;
  ScoreAdditionAnimationStartTime = CurrentTime;
  LastRemainingAnimatedScoreShown = 0;
  ScoreAdditionLastPhase = 0xFF;
  JackpotScoreAnimationType = animationType;

  if (playTick) {
    PlayScoreAnimationTick = 10000;
    if (scoreToAnimate<=10000) {
      PlayScoreAnimationTick = 1000;
      PlayTickLevel = 1;
    } else if (scoreToAnimate<=25000) {
      PlayScoreAnimationTick = 5000;
      PlayTickLevel = 2;
    } else if (scoreToAnimate<=50000) {
      PlayScoreAnimationTick = 10000;
      PlayTickLevel = 3;
    } else if (scoreToAnimate<=100000) {
      PlayScoreAnimationTick = 10000;
      PlayTickLevel = 4;
    } else if (scoreToAnimate<=250000) {
      PlayScoreAnimationTick = 25000;
      PlayTickLevel = 5;
    } else {
      PlayScoreAnimationTick = scoreToAnimate / 9;
      PlayTickLevel = 6;
    }
  } else {
    PlayScoreAnimationTick = 1;
    PlayTickLevel = 0;
    if (JackpotScoreAnimationType==DISPLAY_JACKPOT_ANIMATION_MAJOR_TICKS) {
      PlayScoreAnimationTick = scoreToAnimate / 9;
    }
  }
  
}

unsigned long pow10[10] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

#define MILLISECONDS_PER_FRAME  80
byte LastOtherScoresPhaseShown = 0xFF;

void ShowOtherScores(byte displayNum) {
  if (CurrentNumPlayers<3) return;

  byte frameCheck = (CurrentTime / MILLISECONDS_PER_FRAME);

  if (frameCheck != LastOtherScoresPhaseShown) {
    LastOtherScoresPhaseShown = frameCheck;
  
    // Each score will have this many phases:
    // (DISPLAY_NUM_DIGITS +  (DISPLAY_NUM_DIGITS-2)) to show the player num wipe left to right and then right to left.
    // With two digits remaining, the score will begin to scroll in
    // up to 9 digits to show the score up to 999,999,999 
    // calculated by Display_MagnitudeOfScore
    byte numPhases = 0;
    byte playerNumPhases[4];
    byte numDigitsForScore[4];
    for (byte count=0; count<CurrentNumPlayers; count++) {
      if (count==CurrentPlayer) {
        playerNumPhases[count] = 0;
        numDigitsForScore[count] = 2;
        continue;      
      }
      byte scoreNumDigits = Display_MagnitudeOfScore(CurrentScores[count]);
      if (scoreNumDigits==0) scoreNumDigits = 2;
      numDigitsForScore[count] = scoreNumDigits;
      playerNumPhases[count] = ((DISPLAY_NUM_DIGITS*2)-2 + scoreNumDigits) + 12;
      
      numPhases += playerNumPhases[count];
    }
  
    // This function will cycle through player numbers & scores in the given display
    //unsigned long fullCycleMS = ((unsigned long)numPhases) * 80;
    byte currentPhase = (CurrentTime/MILLISECONDS_PER_FRAME)%numPhases;
  
    byte displayPlayer = 0;
    for (displayPlayer = 0; displayPlayer < CurrentNumPlayers; displayPlayer++) {
      if (currentPhase >= playerNumPhases[displayPlayer]) {
        currentPhase -= playerNumPhases[displayPlayer];
      } else {
        break;
      }
    }

    byte displayMask = 0x00;
    if (currentPhase<((DISPLAY_NUM_DIGITS*2)-2)) {
      unsigned long playerNumRepeated = (RPU_OS_MAX_DISPLAY_SCORE/9) * (displayPlayer+1);
      byte numDigitsToShow = 0;
      if (currentPhase<DISPLAY_NUM_DIGITS) {
        numDigitsToShow = currentPhase+1;
      } else {
        numDigitsToShow = 2*DISPLAY_NUM_DIGITS - (1+currentPhase);
      }
      for (byte count=0; count<numDigitsToShow; count++) {
        displayMask *= 2;
        displayMask |= 1;
      }
      RPU_SetDisplayBlank(displayNum, displayMask);
      RPU_SetDisplay(displayNum, playerNumRepeated, false, 1);
    } else if (currentPhase==((DISPLAY_NUM_DIGITS*2)-2)) {
      byte displayMask = 0x01;
      displayMask |= (0x80 >> (8-DISPLAY_NUM_DIGITS));
      unsigned long scoreToShow = (displayPlayer+1) * (unsigned long)pow10[DISPLAY_NUM_DIGITS-1];
      scoreToShow += (CurrentScores[displayPlayer] / ((unsigned long)pow10[numDigitsForScore[displayPlayer]-1]));
      RPU_SetDisplay(displayNum, scoreToShow, false, 1);
      RPU_SetDisplayBlank(displayNum, displayMask);
    } else if (currentPhase < (playerNumPhases[displayPlayer]-12)) {
      unsigned long scoreToShow = CurrentScores[displayPlayer];
      unsigned long scoreDivisor = (unsigned long)pow10[numDigitsForScore[displayPlayer] - 2 - (currentPhase-11)];
      scoreToShow /= scoreDivisor;
      RPU_SetDisplay(displayNum, scoreToShow, true, 2);
    } else {
      RPU_SetDisplay(displayNum, CurrentScores[displayPlayer], true, 2);      
    }
  }
    
}

unsigned long LastScoreReport = 0;


#ifdef RPU_DMD_DISPLAYS


byte DMD_DisplayNumToID[] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E};

void DMD_FlushEventBuffer(byte displayID) {
  Wire.beginTransmission(displayID);

  // Flush Event Buffer
  Wire.write(0x08);
  Wire.write(0x00);
  
  Wire.endTransmission();
}

void DMD_InitDisplay(byte displayID) {

  Wire.beginTransmission(displayID);
  // Turn Bally Interface Off
  Wire.write(0x06);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission();

  // Flush Event Buffer
  Wire.beginTransmission(displayID);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();

  // Turn off intensity control
  Wire.beginTransmission(displayID);
  Wire.write(0x03);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(0x00);  
  Wire.endTransmission();
}

void DMD_BlankDisplayTemporarily(byte displayID, byte milliseconds) {

  Wire.beginTransmission(displayID);
  Wire.write(0x07); // send Event
  Wire.write(0x0B); // blank event
  Wire.write(milliseconds); // number of ms to blank
  Wire.write(0x20);
  Wire.write(0x01);
  Wire.write(0xFF);
  
  Wire.endTransmission();

}

void DMD_RollBlank(byte displayID) {

  Wire.beginTransmission(displayID);
  Wire.write(0x07); // send Event
  Wire.write(0x0A); // blank event
  Wire.write(0x03); 
  Wire.write(0x00);
  Wire.write(0x01);
  Wire.write(0xFF);
  
  Wire.endTransmission();

}


// Version that uses Wire library to talk to i2c-connected displays
void DMD_WriteDisplay(byte displayID, unsigned long numberValue, byte blankDigit = 0xFF) {
  Wire.beginTransmission(displayID);
  Wire.write(0x02);

  if (numberValue==0) {
    Wire.write(0x30);
    Wire.write(0x30);
  }
  char backwardsBuf[32];
  byte backwardsBufSize = 0;
  while (numberValue) {
    backwardsBuf[backwardsBufSize] = 0x30 + (numberValue%10);
    backwardsBufSize += 1;
    numberValue /= 10;
  }

  for (byte count=0; count<backwardsBufSize; count++) {
    if (blankDigit!=0xFF && count==blankDigit) Wire.write(0x20);
    else Wire.write(backwardsBuf[(backwardsBufSize-1)-count]);
  }

  Wire.write(0x00);
  Wire.endTransmission();
}

// Version that uses Wire library to talk to i2c-connected displays
void DMD_WriteDisplay(byte displayID, const char *message) {
  if (message==NULL) return;
  
  Wire.beginTransmission(displayID);
  byte messageIndex = 0;
  Wire.write(0x02);
  while (message[messageIndex]!=0x00 && message[messageIndex]!='\n' && message[messageIndex]!='\r') {
    Wire.write(message[messageIndex]);
    messageIndex += 1;
  }
  Wire.write(0x00);
  Wire.endTransmission();
}

// Version that uses Wire library to talk to i2c-connected displays
void Display_OverrideScoreDisplay(byte displayNum, const char *message, byte animateEffect, byte overrideMask) {

  if (displayNum > (DISPLAY_NUMBER_OF_PLAYER_DISPLAYS-1)) return;

  ScoreOverrideStatus |= (0x01 << displayNum);
  ScoreAnimation[displayNum] = animateEffect;
//  ScoreOverrideValue[displayNum] = value;
  DMD_WriteDisplay(DMD_DisplayNumToID[displayNum], message);
  LastAnimationSeed[displayNum] = 255;
  OverrideMask[displayNum] = overrideMask;
  
}

unsigned long LastTimeScoreSent = 0;
unsigned long LastTimeDisplayUpdated = 0;

// Version that uses Wire library to talk to i2c-connected displays
void Display_SetDisplayVisible(byte displayNum, boolean visible, unsigned long setScore, byte blankDigit) {

  if (displayNum>=RPU_NUMBER_OF_PLAYER_DISPLAYS) return;

  byte i2cID = (displayNum<5) ? DMD_DisplayNumToID[displayNum] : 0x0A;
  byte scoreNum = 0;
  if (displayNum<RPU_NUMBER_OF_PLAYERS_ALLOWED) scoreNum = displayNum;
  unsigned long scoreToShow = CurrentScores[scoreNum];
  if (setScore!=0xFFFFFFFF) scoreToShow = setScore;
  
  if (visible) DMD_WriteDisplay(i2cID, scoreToShow, blankDigit);
  else DMD_WriteDisplay(i2cID, " ");

}

// Version that uses Wire library to talk to i2c-connected displays
byte Display_UpdateDisplays(byte displayNum, boolean finishAnimation, boolean flashCurrent, byte dashCurrent, unsigned long allScoresShowValue) {
  boolean playTick = 0;

  if (finishAnimation) {
    CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
    ScoreAdditionAnimationStartTime = 0;
    ScoreAdditionAnimation = 0;
  }

  if (LastTimeScoreSent==0) {
    LastTimeScoreSent = 1;
    Wire.begin();
    DMD_InitDisplay(DMD_DisplayNumToID[0]);
    DMD_InitDisplay(DMD_DisplayNumToID[1]);
    DMD_InitDisplay(DMD_DisplayNumToID[2]);
    DMD_InitDisplay(DMD_DisplayNumToID[3]);
    DMD_WriteDisplay(DMD_DisplayNumToID[0], "Trident");
    DMD_WriteDisplay(DMD_DisplayNumToID[1], "Version: 2025");
    DMD_WriteDisplay(DMD_DisplayNumToID[2], "RPU");
    DMD_WriteDisplay(DMD_DisplayNumToID[3], "by Dick Hamill");
   
  }

  if (allScoresShowValue!=0xFFFFFFFF) {
    DMD_WriteDisplay(0x0A, allScoresShowValue);
    DMD_WriteDisplay(0x0B, allScoresShowValue);
    DMD_WriteDisplay(0x0C, allScoresShowValue);
    DMD_WriteDisplay(0x0D, allScoresShowValue);
  } else {

    if (LastTimeScoreChanged>LastTimeScoreSent || flashCurrent || dashCurrent) {      
      LastTimeScoreSent = LastTimeScoreChanged;
      byte overrideBit = 0x01;

      for (byte count=0; count<4; count++) {

        // if this display has been overridden, don't update it...
        if (ScoreOverrideStatus & overrideBit) {
          overrideBit *= 2;
          continue;
        } else {
          overrideBit *= 2;
        }

        if (displayNum==0xFF || displayNum==count) {
          // Send this score to the display (or turn it off)
          if (CurrentNumPlayers && count>(CurrentNumPlayers-1)) {
            Display_SetDisplayVisible(count, false);
          } else {

            if (count==CurrentPlayer && (flashCurrent || dashCurrent)) {
              unsigned long flashSeed = CurrentTime / 500;
              if (flashSeed != LastFlashOrDash) {
                LastFlashOrDash = flashSeed;
                if (flashCurrent) {
                  if ((LastFlashOrDash % 2) == 0) DMD_BlankDisplayTemporarily(DMD_DisplayNumToID[count], 10);
                  else DMD_FlushEventBuffer(DMD_DisplayNumToID[count]);
                } else if (dashCurrent) {
//                  byte blankDigit = LastFlashOrDash%20;
//                  Display_SetDisplayVisible(count, true, CurrentScores[count], blankDigit);
                  if ((LastFlashOrDash % 4) == 0) DMD_RollBlank(DMD_DisplayNumToID[count]);
                  else DMD_FlushEventBuffer(DMD_DisplayNumToID[count]);
                }
              }
            } else {              
              Display_SetDisplayVisible(count, true, CurrentScores[count]);  
            }
          }
        }
        
      }
  
    }
  }
  return playTick;  
}


#else


// Version that uses RPU libraries to talk to MPU-connected displays
void Display_SetDisplayVisible(byte displayNum, boolean visible, unsigned long setScore = 0xFFFFFFFF) {

  if (displayNum>=RPU_NUMBER_OF_PLAYER_DISPLAYS) return;

  if (visible) {
    if (setScore==0xFFFFFFFF) RPU_SetDisplay(displayNum, CurrentScores[displayNum]);
    else RPU_SetDisplay(displayNum, CurrentScores[displayNum], true, 2, true);
  } else {
    RPU_SetDisplayBlank(displayNum, 0x00);
  }
}

// Version that uses RPU libraries to talk to MPU-connected dislays:
byte Display_UpdateDisplays(byte displayNum, boolean finishAnimation, boolean flashCurrent, byte dashCurrent, unsigned long allScoresShowValue) {
  boolean playTick = 0;

  if (finishAnimation) {
    CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
    ScoreAdditionAnimationStartTime = 0;
    ScoreAdditionAnimation = 0;
  }

  if (DisplayAchievements && allScoresShowValue==0xFFFFFFFF) {
    if (allScoresShowValue==0xFFFFFFFF) {
      if (displayNum==0xFF) {
        for (byte count=0; count<RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
          CurrentAchievements[count] = CurrentScores[count]%10;
        }
      } else {
        CurrentAchievements[displayNum] = CurrentScores[displayNum]%10;
      }
    } else {
      for (byte count=0; count<RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
        CurrentAchievements[count] = allScoresShowValue;
      }
    }
  }

  for (byte displayCount = 0; displayCount < DISPLAY_NUMBER_OF_PLAYER_DISPLAYS; displayCount++) {

    byte adjDisplayCount = displayCount;
    if (CurrentPlayer>=DISPLAY_NUMBER_OF_PLAYER_DISPLAYS) {
      adjDisplayCount = displayCount + DISPLAY_NUMBER_OF_PLAYER_DISPLAYS;
    }
    
    // Don't change a display unless it's requested
    if (displayNum!=0xFF && adjDisplayCount!=displayNum) continue;

    // Determine what we're showing
    // 1) score addition animation
    // 2) override animation
    // 3) regular scores
    if (ScoreAdditionAnimationStartTime && (CurrentPlayer==adjDisplayCount)) {
      if (JackpotScoreAnimationType == DISPLAY_JACKPOT_ANIMATION_OFF) {
        LastRemainingAnimatedScoreShown = 0;
        CurrentScores[displayCount] += ScoreAdditionAnimation;
        ScoreAdditionAnimationStartTime = 0;
        ScoreAdditionAnimation = 0;        
      } else if (JackpotScoreAnimationType == DISPLAY_JACKPOT_ANIMATION_ROLLING) {
        // Show score addition animation
        if (CurrentTime < (ScoreAdditionAnimationStartTime + 1500)) {
          byte scoreAnimationPhase = (CurrentTime - ScoreAdditionAnimationStartTime) / 100;
          if (scoreAnimationPhase!=ScoreAdditionLastPhase) {
            if ((scoreAnimationPhase%2)==0) {
              RPU_SetDisplayBlank(displayCount, 0x00);
            } else {
              playTick = 1;
              unsigned long scoreToShow = ScoreAdditionAnimation;
              if (scoreToShow) {
                byte numberOfDigits = Display_MagnitudeOfScore(scoreToShow);
                if (scoreAnimationPhase<((numberOfDigits-1)*2)) {
                  scoreToShow /= (unsigned long)(pow10[numberOfDigits - 1 - (scoreAnimationPhase/2)]);
                }
              }
              RPU_SetDisplay(displayCount, scoreToShow, true, 1);
              
            }
            ScoreAdditionLastPhase = scoreAnimationPhase;
          }
          
        } else if (CurrentTime < (ScoreAdditionAnimationStartTime + 3000)) {
          unsigned long remainingScore = 0;
          remainingScore = (((CurrentTime - ScoreAdditionAnimationStartTime) - 1500) * ScoreAdditionAnimation) / 3000;
          ShowPlayerScore(CurrentScores[displayCount] + remainingScore, displayCount);
          if (PlayScoreAnimationTick>1 && (remainingScore / PlayScoreAnimationTick) != (LastRemainingAnimatedScoreShown / PlayScoreAnimationTick)) {
            LastRemainingAnimatedScoreShown = remainingScore;
            playTick = PlayTickLevel + 1;
          }
        } else {
          LastRemainingAnimatedScoreShown = 0;
          CurrentScores[displayCount] += ScoreAdditionAnimation;
          ScoreAdditionAnimationStartTime = 0;
          ScoreAdditionAnimation = 0;
        }
      } else if (JackpotScoreAnimationType == DISPLAY_JACKPOT_ANIMATION_MAJOR_TICKS) {
        byte scoreAnimationPhase = (CurrentTime - ScoreAdditionAnimationStartTime) / 250;
        if (scoreAnimationPhase!=ScoreAdditionLastPhase) {
          ScoreAdditionLastPhase = scoreAnimationPhase;
          playTick = PlayTickLevel;
          if (ScoreAdditionAnimation > PlayScoreAnimationTick) {
            CurrentScores[displayCount] += PlayScoreAnimationTick;
            ScoreAdditionAnimation -= PlayScoreAnimationTick;
          } else {
            LastRemainingAnimatedScoreShown = 0;
            CurrentScores[displayCount] += ScoreAdditionAnimation;
            ScoreAdditionAnimationStartTime = 0;
            ScoreAdditionAnimation = 0;
          }
          ShowPlayerScore(CurrentScores[displayCount], displayCount);
        }
      }
      
    } else if (ScoreOverrideStatus & (0x01<<displayCount))  {
      // Show override value
      unsigned long displayScore = ScoreOverrideValue[displayCount];
      if (displayScore != DISPLAY_OVERRIDE_BLANK_SCORE) {
        ShowAnimatedValue(displayCount, displayScore, ScoreAnimation[displayCount]);
      } else {
        RPU_SetDisplayBlank(displayCount, 0);
      }
    } else {

      // if we're showing the high score, just show that
      if (allScoresShowValue!=0xFFFFFFFF) {
        ShowPlayerScore(allScoresShowValue, displayCount);
      } else if (CurrentNumPlayers && displayCount>(CurrentNumPlayers-1)) {
        // We're not showing a high score and there is no player this high, so blank the display
        RPU_SetDisplayBlank(displayCount, 0x00);
      } else if (CurrentNumPlayers <= DISPLAY_NUMBER_OF_PLAYER_DISPLAYS) {
        // No need to juggle anything because all the player scores will
        // fit on the number of displays we have
        if (CurrentPlayer==displayCount) ShowPlayerScore(CurrentScores[displayCount], displayCount, flashCurrent, dashCurrent);
        else ShowPlayerScore(CurrentScores[displayCount], displayCount);
      } else {
        if (CurrentPlayer==adjDisplayCount) {
          // We're showing the currentScore in this display
          ShowPlayerScore(CurrentScores[CurrentPlayer], displayCount, flashCurrent, dashCurrent);
          if (CurrentTime > (LastScoreReport+1000)) {
            LastScoreReport = CurrentTime;
//            char buf[128];
//            sprintf(buf, "Score=%lu, display count=%d\n", CurrentScores[CurrentPlayer], displayCount);
//            Serial.write(buf);
          }
        } else {
          // We're looping through and showing other player's scores in this display
          ShowOtherScores(displayCount);
        }
      }
    }
    
  }

  return playTick;
}
#endif
