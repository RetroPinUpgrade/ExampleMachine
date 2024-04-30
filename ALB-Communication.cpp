#include "Arduino.h"
#include "ALB-Communication.h"

byte ALBMessageCounter = 0;
byte ALBMessageExpectedLength = 0;
byte *ALBMessage = NULL;
void (*ALBIncomingMessageHandler)(byte *) = NULL;


void ProcessIncomingData(byte nextByte) {
  if (ALBMessage==NULL) return;

  if (ALBMessageCounter==0) {
    ALBMessageExpectedLength = 0;
    if (nextByte==ALB_HEADER_BYTE_1) {
      ALBMessage[ALBMessageCounter] = nextByte;
      ALBMessageCounter += 1;
    }
  } else if (ALBMessageCounter==1) {
    if (nextByte==ALB_HEADER_BYTE_2) {
      ALBMessage[ALBMessageCounter] = nextByte;
      ALBMessageCounter += 1;
    } else {
      ALBMessageCounter = 0;
    }
  } else if (ALBMessageCounter==2) {
    ALBMessage[ALBMessageCounter] = nextByte;
    ALBMessageExpectedLength = ALBMessage[ALBMessageCounter];
    ALBMessageCounter += 1;
  } else if (ALBMessageCounter>2 && ALBMessageCounter<(ALBMessageExpectedLength-1)) {
    ALBMessage[ALBMessageCounter] = nextByte;
    ALBMessageCounter += 1;
  } else if (ALBMessageCounter==(ALBMessageExpectedLength-1)) {
    if (nextByte==ALB_END_OF_MESSAGE) {
      ALBMessage[ALBMessageCounter] = nextByte;
      ALBMessageCounter += 1;
      // This is a completed, well-formed message so we can parse it
      ALBIncomingMessageHandler(ALBMessage);
    } else {
      // Something went awry with reception -- start over
    }
    ALBMessageCounter = 0;
  } else {
    ALBMessageCounter = 0;
  }
    
}


void DataRequest() {
}

void DataReceive(int numBytes) {
  while (Wire.available()) {
    ProcessIncomingData(Wire.read());  
  }

  (void)numBytes;
}


AccessoryLampBoard::AccessoryLampBoard() {
  m_targetDeviceAddress = 0;
  m_communicationInitialized = false;
  ALBMessage = NULL;
  ALBIncomingMessageHandler = NULL;
  ALBMessageCounter = 0;
}

AccessoryLampBoard::~AccessoryLampBoard() {  
}

void AccessoryLampBoard::SetTargetDeviceAddress(byte targetDeviceAddress) {
  m_targetDeviceAddress = targetDeviceAddress;
}

void AccessoryLampBoard::InitOutogingCommunication() {
  Wire.begin();
  m_communicationInitialized = true;
}

void AccessoryLampBoard::InitIncomingCommunication(byte incomingDeviceAddress, void (*incomingMessageHandler)(byte *)) {
  ALBIncomingMessageHandler = incomingMessageHandler;
  ALBMessage = (byte *)malloc(ALB_MAX_MESSAGE_LENGTH);
  if (ALBIncomingMessageHandler!=NULL && ALBMessage!=NULL) {
    m_communicationInitialized = true;
    Wire.begin(incomingDeviceAddress);
    Wire.onReceive(DataReceive);
    Wire.onRequest(DataRequest);
    ALBMessage[0] = ALB_END_OF_MESSAGE;
  }
}


boolean AccessoryLampBoard::EnableLamps() {
  if (!m_communicationInitialized) return false;

  Wire.beginTransmission(m_targetDeviceAddress);
  Wire.write(ALB_HEADER_BYTE_1);
  Wire.write(ALB_HEADER_BYTE_2);
  Wire.write(5); // length of messages
  Wire.write(ALB_COMMAND_ENABLE_LAMPS);
  Wire.write(ALB_END_OF_MESSAGE);
  Wire.endTransmission();
  return true;
}


boolean AccessoryLampBoard::DisableLamps() {
  if (!m_communicationInitialized) return false;

  Wire.beginTransmission(m_targetDeviceAddress);
  Wire.write(ALB_HEADER_BYTE_1);
  Wire.write(ALB_HEADER_BYTE_2);
  Wire.write(5); // length of messages
  Wire.write(ALB_COMMAND_DISABLE_LAMPS);
  Wire.write(ALB_END_OF_MESSAGE);
  Wire.endTransmission();
  return true;
}



boolean AccessoryLampBoard::PlayAnimation(byte animationNum) {
  if (!m_communicationInitialized) return false;

  Wire.beginTransmission(m_targetDeviceAddress);
  Wire.write(ALB_HEADER_BYTE_1);
  Wire.write(ALB_HEADER_BYTE_2);
  Wire.write(6); // length of messages
  Wire.write(ALB_COMMAND_PLAY_ANIMATION);
  Wire.write(animationNum);
  Wire.write(ALB_END_OF_MESSAGE);
  Wire.endTransmission();

  return true;
}

boolean AccessoryLampBoard::LoopAnimation(byte animationNum) {
  if (!m_communicationInitialized) return false;

  Wire.beginTransmission(m_targetDeviceAddress);
  Wire.write(ALB_HEADER_BYTE_1);
  Wire.write(ALB_HEADER_BYTE_2);
  Wire.write(6); // length of messages
  Wire.write(ALB_COMMAND_LOOP_ANIMATION);
  Wire.write(animationNum);
  Wire.write(ALB_END_OF_MESSAGE);
  Wire.endTransmission();
  
  return true;
}

boolean AccessoryLampBoard::StopAnimation(byte animationNum) {
  if (!m_communicationInitialized) return false;

  if (animationNum==ALB_ALL_ANIMATIONS) {
    Wire.beginTransmission(m_targetDeviceAddress);
    Wire.write(ALB_HEADER_BYTE_1);
    Wire.write(ALB_HEADER_BYTE_2);
    Wire.write(5); // length of messages
    Wire.write(ALB_COMMAND_STOP_ALL_ANIMATIONS);
    Wire.write(ALB_END_OF_MESSAGE);
    Wire.endTransmission();
  } else {
    Wire.beginTransmission(m_targetDeviceAddress);
    Wire.write(ALB_HEADER_BYTE_1);
    Wire.write(ALB_HEADER_BYTE_2);
    Wire.write(6); // length of messages
    Wire.write(ALB_COMMAND_STOP_ANIMATION);
    Wire.write(animationNum);
    Wire.write(ALB_END_OF_MESSAGE);
    Wire.endTransmission();
  }

  return true;
}



boolean AccessoryLampBoard::AllLampsOff() {
  if (!m_communicationInitialized) return false;

  Wire.beginTransmission(m_targetDeviceAddress);
  Wire.write(ALB_HEADER_BYTE_1);
  Wire.write(ALB_HEADER_BYTE_2);
  Wire.write(5); // length of messages
  Wire.write(ALB_COMMAND_ALL_LAMPS_OFF);
  Wire.write(ALB_END_OF_MESSAGE);
  Wire.endTransmission();
  
  return true;
}
