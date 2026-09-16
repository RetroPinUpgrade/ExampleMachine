#include <Wire.h>

#ifndef ALB_COMMUNICATION_H

#define ALB_HEADER_BYTE_1	0xF0
#define ALB_HEADER_BYTE_2	0xBB
#define ALB_END_OF_MESSAGE	0x55

#define ALB_MAX_MESSAGE_LENGTH  250

#define ALB_COMMAND_GET_ACCESSORY_ID              0
#define ALB_COMMAND_ENABLE_LAMPS                  1
#define ALB_COMMAND_DISABLE_LAMPS                 2
#define ALB_COMMAND_PLAY_ANIMATION                3
#define ALB_COMMAND_LOOP_ANIMATION                4
#define ALB_COMMAND_STOP_ANIMATION                5
#define ALB_COMMAND_STOP_ALL_ANIMATIONS           6
#define ALB_COMMAND_PLAY_WITH_COLOR               7
#define ALB_COMMAND_PLAY_WITH_COLOR_AND_DURATION  8
#define ALB_COMMAND_LOOP_WITH_COLOR               9
#define ALB_COMMAND_LOOP_WITH_COLOR_AND_DURATION  10
#define ALB_COMMAND_ADJUST_SETTING                11
#define ALB_COMMAND_WATCHDOG_TIMER_RESET          12
#define ALB_COMMAND_STOP_ANIMATION_BY_AREA        13
#define ALB_COMMAND_REQUEST_SETTING               14
#define ALB_COMMAND_SET_LAMP_STATE                15
#define ALB_COMMAND_FLASH_ALL_LAMPS               16
#define ALB_COMMAND_TURN_OFF_ALL_LAMPS            17
#define ALB_COMMAND_SET_DIM_DIVISOR               18
#define ALB_COMMAND_ENABLE_BS_EMULATION           19
#define ALB_COMMAND_DISABLE_BS_EMULATION          20
#define ALB_COMMAND_ALL_LAMPS_OFF                 99

#define ALB_ALL_ANIMATIONS                0xFF

#define ALB_WATCHDOG_TIMER_DURATION               500

class AccessoryLampBoard {

  public:
    AccessoryLampBoard();
    ~AccessoryLampBoard();

    void InitOutogingCommunication();
    void SetTargetDeviceAddress(byte targetDeviceAddress);
    void InitIncomingCommunication(byte incomingDeviceAddress, void (*incomingMessageHandler)(byte *));
    void SetRequestedSettingValue(byte value);

    boolean EnableLamps();
    boolean WatchdogTimerReset();
    boolean DisableLamps();
    boolean PlayAnimation(byte animationNum);
    boolean PlayAnimation(byte animationNum, byte red, byte green, byte blue);
    boolean PlayAnimation(byte animationNum, byte red, byte green, byte blue, short duration);
    boolean LoopAnimation(byte animationNum);
    boolean LoopAnimation(byte animationNum, byte red, byte green, byte blue);
    boolean LoopAnimation(byte animationNum, byte red, byte green, byte blue, short duration);
    boolean StopAnimation(byte animationNum = ALB_ALL_ANIMATIONS);
    boolean StopAnimationByArea(byte areaNum);
    boolean AdjustSetting(byte settingNum, byte value1, byte value2, byte value3);
    boolean RequestSettingValue(byte settingNum, byte *returnedValue);
    boolean AllLampsOff();

  private:

    byte      m_targetDeviceAddress;
    boolean   m_communicationInitialized;
};


#define ALB_COMMUNICATION_H
#endif
