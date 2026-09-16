
#define LAMP_BONUS_1K                 0  // Q14, J1-18
#define LAMP_BONUS_2K                 1 // Q29, J1-1
#define LAMP_BONUS_3K                 2 // Q36, J3-26
#define LAMP_BONUS_4K                 3 // Q57, J3-1
#define LAMP_BONUS_5K                 4 // Q12, J1-19
#define LAMP_BONUS_6K                 5 // Q27, J1-9
#define LAMP_BONUS_7K                 6 // Q38, J3-25
#define LAMP_BONUS_8K                 7 // Q50, J3-12
#define LAMP_BONUS_9K                 8 // Q13, J1-17
#define LAMP_BONUS_10K                9 // Q28, J1-8
#define LAMP_BONUS_20K                10  // Q44, J3-19
//#define LAMP_                       12  // Q8, J1-23
//#define LAMP_                       13  // Q35, J1-3
//#define LAMP_                       14  // Q49, J3-17
//#define LAMP_                       15  // Q54, J3-11
//#define LAMP_                       16  // Q9, J1-14
//#define LAMP_                       18  // Q48, J3-16
//#define LAMP_                       19  // Q55, J3-9
//#define LAMP_                       21  // Q22, J1-10
//#define LAMP_                       22  // Q37, J3-23
//#define LAMP_                       23  // Q60, J3-3
//#define LAMP_                       24  // Q11, J1-16
//#define LAMP_                       25  // Q26, J1-7
//#define LAMP_                       26  // Q32, J3-27
//#define LAMP_                       27  // Q59, J3-4
//#define LAMP_                       28  // Q4, J1-28
//#define LAMP_                       29  // Q25, J1-6
//#define LAMP_                       30  // Q20, J1-13
//#define LAMP_                       32  // Q1, J1-24
//#define LAMP_                       33  // Q24, J1-5
//#define LAMP_                       34  // Q42, J3-21
//#define LAMP_                       35  // Q56, J3-10
//#define LAMP_                       38  // Q41, J3-20
//#define LAMP_                       39  // Q46, J3-18
#define LAMP_HEAD_MATCH               41  // Q23, J2-8
#define LAMP_SHOOT_AGAIN              42  // Q40, J2-9
#define LAMP_APRON_CREDIT             43  // Q52, J2-5
#define LAMP_RIGHT_INLANE             44  // Q7, J2-13
#define LAMP_LEFT_INLANE              45  // Q21, J2-12
#define LAMP_RIGHT_OUTLANE            46  // Q39, J2-4
#define LAMP_LEFT_OUTLANE             47  // Q53, J2-3
#define LAMP_BALL_IN_PLAY             48  // Q16, J2-22
#define LAMP_HEAD_HIGH_SCORE          49  // Q15, J2-23
#define LAMP_HEAD_GAME_OVER           50  // Q33, J2-11
#define LAMP_HEAD_TILT                51  // Q47, J2-10
#define LAMP_HEAD_1_PLAYER            52  // Q5, J2-16
#define LAMP_HEAD_2_PLAYERS           53  // Q18, J2-20
#define LAMP_HEAD_3_PLAYERS           54  // Q30, J2-6
#define LAMP_HEAD_4_PLAYERS           55  // Q43, J2-7
#define LAMP_HEAD_PLAYER_1_UP         56  // Q6, J2-14
#define LAMP_HEAD_PLAYER_2_UP         57  // Q19, J2-15
#define LAMP_HEAD_PLAYER_3_UP         58  // Q31, J2-2
#define LAMP_HEAD_PLAYER_4_UP         59  // Q45, J2-1


#define SW_LEFT_SPINNER             0
#define SW_RIGHT_SPINNER            1
#define SW_CREDIT_RESET             5

// These may have different switches, or they might be the same
#define SW_TILT                     6
#define SW_PLUMB_TILT               6
#define SW_ROLL_TILT                6

#define SW_OUTHOLE                  7
#define SW_COIN_3                   8
#define SW_COIN_1                   9
#define SW_COIN_2                   10
#define SW_SLAM                     15
#define SW_DROP_R_1                 16
#define SW_DROP_R_2                 17
#define SW_DROP_R_3                 18
#define SW_LEFT_SLING               19
#define SW_RIGHT_SLING              20
#define SW_DROP_L_1                 24
#define SW_DROP_L_2                 25
#define SW_DROP_L_3                 26
#define SW_POP_BUMPER               31
#define SW_SAUCER                   32
#define SW_LEFT_OUTLANE             34
#define SW_LEFT_INLANE              35
#define SW_RIGHT_OUTLANE            37
#define SW_RIGHT_INLANE             36

#define SOL_POP_BUMPER              6
#define SOL_DROP_BANK_R_RESET       2
#define SOL_DROP_BANK_L_RESET       3
#define SOL_KNOCKER                 4
#define SOL_SAUCER                  7
#define SOL_LEFT_SLING              10
#define SOL_RIGHT_SLING             11
#define SOL_SERVE_BALL              8


// These SolenoidAssociatedSwitches are only
// used for Bally/Stern. On Williams, the "immediate solenoids"
// are activated in hardware so they don't need this
// code
#if (RPU_MPU_ARCHITECTURE<10)
#define NUM_SWITCHES_WITH_TRIGGERS          3 // total number of solenoid/switch pairs
#define NUM_PRIORITY_SWITCHES_WITH_TRIGGERS 3 // This number should match the define above

struct PlayfieldAndCabinetSwitch SolenoidAssociatedSwitches[] = {
  { SW_RIGHT_SLING, SOL_RIGHT_SLING, 4},
  { SW_LEFT_SLING, SOL_LEFT_SLING, 4},
  { SW_POP_BUMPER, SOL_POP_BUMPER, 4}
};
#endif




#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
#define MESSAGE_AREA_TOPPER                       1
#define MESSAGE_AREA_SPEAKER                      2
#define MESSAGE_AREA_LEFT_SPEAKER                 3
#define MESSAGE_AREA_RIGHT_SPEAKER                4
#define MESSAGE_AREA_STADIUM                      5
#define MESSAGE_AREA_UNDERCAB                     6
#define MESSAGE_AREA_BACKGLASS                    7
#define MESSAGE_AREA_GI_0                         8
#define MESSAGE_AREA_GI_1                         9
#define MESSAGE_AREA_GI_2                         10
#define MESSAGE_AREA_GI_3                         11
#define MESSAGE_AREA_GI_4                         12
#define SET_COLOR_0                               0
#define SET_COLOR_1                               1
#define SET_COLOR_2                               2
#define SET_TOPPER_BRIGHTNESS                     3
#define SET_SPEAKER_BRIGHTNESS                    4
#define SET_BACKGLASS_BRIGHTNESS                  5
#define SET_STADIUM_BRIGHTNESS                    6
#define SET_UNDERCAB_BRIGHTNESS                   7
#define SET_GI_BRIGHTNESS_0                       8
#define SET_GI_BRIGHTNESS_1                       9
#define SET_GI_BRIGHTNESS_2                       10
#define SET_GI_BRIGHTNESS_3                       11
#define SET_GI_BRIGHTNESS_4                       12
#define TOPPER_PULSE_COLOR_0                      15
#define TOPPER_PULSE_COLOR_1                      16
#define TOPPER_PULSE_COLOR_2                      17
#define TOPPER_FLASH_COLOR_0                      18
#define TOPPER_FLASH_COLOR_1                      19
#define TOPPER_FLASH_COLOR_2                      20
#define TOPPER_LOOP_COLOR_0                       21
#define TOPPER_LOOP_COLOR_1                       22
#define TOPPER_LOOP_COLOR_2                       23
#define TOPPER_LEFT_FLASH_COLOR_0                 24
#define TOPPER_LEFT_FLASH_COLOR_1                 25
#define TOPPER_LEFT_FLASH_COLOR_2                 26
#define TOPPER_RIGHT_FLASH_COLOR_0                27
#define TOPPER_RIGHT_FLASH_COLOR_1                28
#define TOPPER_RIGHT_FLASH_COLOR_2                29
#define TOPPER_LEFT_TO_RIGHT_COLOR_0              30
#define TOPPER_LEFT_TO_RIGHT_COLOR_1              31
#define TOPPER_LEFT_TO_RIGHT_COLOR_2              32
#define TOPPER_RIGHT_TO_LEFT_COLOR_0              33
#define TOPPER_RIGHT_TO_LEFT_COLOR_1              34
#define TOPPER_RIGHT_TO_LEFT_COLOR_2              35
#define TOPPER_SPARKLE_COLOR_0                    36
#define TOPPER_SPARKLE_COLOR_1                    37
#define TOPPER_SPARKLE_COLOR_2                    38
#define TOPPER_FIRE                               39
#define TOPPER_CANDY_PULSE                        40
#define TOPPER_LIGHTNING_0                        41
#define TOPPER_LIGHTNING_1                        42
#define TOPPER_LIGHTNING_2                        43
#define TOPPER_LIGHTNING_3                        44
#define TOPPER_LIGHTNING_4                        45
#define TOPPER_BIG_LIGHTNING_0                    46
#define TOPPER_BIG_LIGHTNING_1                    47
#define LEFT_TO_RIGHT_SPEAKER_COLOR_0             50
#define LEFT_TO_RIGHT_SPEAKER_COLOR_1             51
#define LEFT_TO_RIGHT_SPEAKER_COLOR_2             52
#define RIGHT_TO_LEFT_SPEAKER_COLOR_0             53
#define RIGHT_TO_LEFT_SPEAKER_COLOR_1             54
#define RIGHT_TO_LEFT_SPEAKER_COLOR_2             55
#define BOTH_SPEAKERS_TWO_COLOR_0_1_PULSE         56
#define LEFT_SPEAKER_PULSE_COLOR_0                60
#define LEFT_SPEAKER_PULSE_COLOR_1                61
#define LEFT_SPEAKER_PULSE_COLOR_2                62
#define LEFT_SPEAKER_FLASH_COLOR_0                63
#define LEFT_SPEAKER_FLASH_COLOR_1                64
#define LEFT_SPEAKER_FLASH_COLOR_2                65
#define LEFT_SPEAKER_LOOP_CW_COLOR_0              66
#define LEFT_SPEAKER_LOOP_CW_COLOR_1              67
#define LEFT_SPEAKER_LOOP_CW_COLOR_2              68
#define LEFT_SPEAKER_LOOP_CCW_COLOR_0             69
#define LEFT_SPEAKER_LOOP_CCW_COLOR_1             70
#define LEFT_SPEAKER_LOOP_CCW_COLOR_2             71
#define RIGHT_SPEAKER_PULSE_COLOR_0               75
#define RIGHT_SPEAKER_PULSE_COLOR_1               76
#define RIGHT_SPEAKER_PULSE_COLOR_2               77
#define RIGHT_SPEAKER_FLASH_COLOR_0               78
#define RIGHT_SPEAKER_FLASH_COLOR_1               79
#define RIGHT_SPEAKER_FLASH_COLOR_2               80
#define RIGHT_SPEAKER_LOOP_CW_COLOR_0             81
#define RIGHT_SPEAKER_LOOP_CW_COLOR_1             82
#define RIGHT_SPEAKER_LOOP_CW_COLOR_2             83
#define RIGHT_SPEAKER_LOOP_CCW_COLOR_0            84
#define RIGHT_SPEAKER_LOOP_CCW_COLOR_1            85
#define RIGHT_SPEAKER_LOOP_CCW_COLOR_2            86
#define STADIUM_SET_COLOR_0                       90
#define STADIUM_SET_COLOR_1                       91
#define STADIUM_SET_COLOR_2                       92
#define STADIUM_PULSE_COLOR_0                     93
#define STADIUM_PULSE_COLOR_1                     94
#define STADIUM_PULSE_COLOR_2                     95
#define UNDERCAB_SET_COLOR_0                      120
#define BACKGLASS_SET_SCENE_0                     170
#define GI_0_SET_COLOR                            210
#define GI_0_PULSE_COLOR                          211
#define GI_0_FLICKER_COLOR                        212
#define GI_1_SET_COLOR                            218
#define GI_1_PULSE_COLOR                          219
#define GI_1_FLICKER_COLOR                        220
#define GI_2_SET_COLOR                            226
#define GI_2_PULSE_COLOR                          227
#define GI_2_FLICKER_COLOR                        228
#define GI_2_FLICKER_START_PIXEL                  229
#define GI_2_FLICKER_END_PIXEL                    230
#define GI_3_SET_COLOR                            232
#define GI_4_SET_COLOR                            240
#define CONTROL_MESSAGE_UNSET                     255
#endif
