
// Display Management Functions

#define DISPLAY_OVERRIDE_BLANK_SCORE 0xFFFFFFFF
#define DISPLAY_OVERRIDE_ANIMATION_NONE     0
#define DISPLAY_OVERRIDE_ANIMATION_BOUNCE   1
#define DISPLAY_OVERRIDE_ANIMATION_FLUTTER  2
#define DISPLAY_OVERRIDE_ANIMATION_FLYBY    3
#define DISPLAY_OVERRIDE_ANIMATION_CENTER   4
#define DISPLAY_OVERRIDE_SYMMETRIC_BOUNCE   5
#define DISPLAY_OVERRIDE_CENTER_FLASH_SLOW  6
#define DISPLAY_OVERRIDE_CENTER_FLASH_FAST  7

#define DISPLAY_DASH_ROLLING_BLANK          1
#define DISPLAY_DASH_INTERMITTENT_FLASH     2

#define DISPLAY_JACKPOT_ANIMATION_OFF           0
#define DISPLAY_JACKPOT_ANIMATION_ROLLING       1
#define DISPLAY_JACKPOT_ANIMATION_MAJOR_TICKS   2

void Display_ResetDisplayTrackingVariables();
byte Display_MagnitudeOfScore(unsigned long score);
void Display_SetAnimationDisplayOrder(byte disp0, byte disp1, byte disp2, byte disp3);

void Display_OverrideScoreDisplay(byte displayNum, unsigned long value, byte animationType, byte overrideMask = 0xFF);
#ifdef RPU_DMD_DISPLAYS
void Display_OverrideScoreDisplay(byte displayNum, const char *message, byte animateEffect, byte overrideMask = 0xFF);
#endif
void Display_ClearOverride(byte displayNum = 0xFF);
void Display_EnableAchievements(boolean achievementsShown = true);
void Display_StartScoreAnimation(unsigned long scoreAdditionValue, boolean playTick, byte animationType=DISPLAY_JACKPOT_ANIMATION_ROLLING);
//void Display_ShowflybyValue(byte numToShow, unsigned long timeBase);

void Display_SetLastTimeScoreChanged(unsigned long scoreChangedTime);
unsigned long Display_GetLastTimeScoreChanged();
byte Display_UpdateDisplays(byte displayNum = 0xFF, boolean finishAnimation = false, boolean flashCurrent = false, byte dashCurrent = false, unsigned long allScoresShowValue = 0xFFFFFFFF);
void Display_SetDisplayVisible(byte displayNum, boolean visible, unsigned long setScore = 0xFFFFFFFF, byte blankDigit = 0xFF);
