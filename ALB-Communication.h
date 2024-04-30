#include <Wire.h>

#ifndef ALB_COMMUNICATION_H

#define ALB_HEADER_BYTE_1	0xF0
#define ALB_HEADER_BYTE_2	0xBB
#define ALB_END_OF_MESSAGE	0x55

#define ALB_MAX_MESSAGE_LENGTH  250

#define ALB_COMMAND_GET_ACCESSORY_ID      0
#define ALB_COMMAND_ENABLE_LAMPS          1
#define ALB_COMMAND_DISABLE_LAMPS         2
#define ALB_COMMAND_PLAY_ANIMATION        3
#define ALB_COMMAND_LOOP_ANIMATION        4
#define ALB_COMMAND_STOP_ANIMATION        5
#define ALB_COMMAND_STOP_ALL_ANIMATIONS   6
#define ALB_COMMAND_ALL_LAMPS_OFF         99

#define ALB_ALL_ANIMATIONS                0xFF

class AccessoryLampBoard {

  public:
    AccessoryLampBoard();
    ~AccessoryLampBoard();

    void InitOutogingCommunication();
    void SetTargetDeviceAddress(byte targetDeviceAddress);
    void InitIncomingCommunication(byte incomingDeviceAddress, void (*incomingMessageHandler)(byte *));

    boolean EnableLamps();
    boolean DisableLamps();
    boolean PlayAnimation(byte animationNum);
    boolean LoopAnimation(byte animationNum);
    boolean StopAnimation(byte animationNum = ALB_ALL_ANIMATIONS);
    boolean AllLampsOff();

  private:

    byte      m_targetDeviceAddress;
    boolean   m_communicationInitialized;
};


#define ALB_COMMUNICATION_H
#endif
