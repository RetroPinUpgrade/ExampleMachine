// This file must be included after RPU_config.h

#define DROP_TARGET_BANK_CLEARED          1
#define DROP_TARGET_BANK_CLEARED_IN_ORDER 2

#define DROP_TARGET_TYPE_BLY_1          0   // no solenoids to drop individual targets, one switch per target
#define DROP_TARGET_TYPE_STRN_1         1   // no solenoids to drop individual targets, one switch per target
#define DROP_TARGET_TYPE_STRN_2         2   // clearing solenoids for individual targets, one switch per target (aka memory drops)
#define DROP_TARGET_TYPE_WLLMS_1        3   // no solenoids to drop individual targets, one switch per target (momentary), switch for all targets down
#define DROP_TARGET_TYPE_WLLMS_2        4   // no solenoids to drop/reset individual, one switch per target

class DropTargetBank
{
  public:
    DropTargetBank(byte s_numSwitches, byte s_numSolenoids, byte s_bankType, byte s_solenoidOnTime);
    ~DropTargetBank();
    void DefineSwitch(byte switchOrder, byte switchNum);
    void DefineResetSolenoid(byte solIndex, byte solChannelNumber);
    void AddAllTargetsSwitch(byte s_allTargetsSwitch);
    byte HandleDropTargetHit(byte switchNum);
    byte CheckIfBankCleared();
    void Update(unsigned long currentTime);
    void ResetDropTargets(unsigned long timeToReset, boolean ignoreQuickDrops=false);
    byte GetStatus();

  private:
    byte numSwitches;
    byte bankType;
    byte numSolenoids;
    byte *switchArray;
    byte *solArray;
    byte allTargetsSwitch;
    byte solenoidOnTime;
    byte bankStatus;
    byte bankBitmask;
    unsigned long targetResetTime;
    unsigned long ignoreDropsUntilTime;
    boolean bankCleared;
    boolean targetsHitInOrder;
    byte numTargetsInOrder;
};

DropTargetBank::DropTargetBank(byte s_numSwitches, byte s_numSolenoids, byte s_bankType, byte s_solenoidOnTime) {
  numSwitches = s_numSwitches;
  numSolenoids = s_numSolenoids;
  solenoidOnTime = s_solenoidOnTime;
  bankType = s_bankType;
  if (numSwitches) switchArray = new byte[numSwitches];
  if (numSolenoids) solArray = new byte[numSolenoids];
  allTargetsSwitch = 0xFF;
  bankStatus = 0;
  bankBitmask = 0;
  bankCleared = false;
  targetsHitInOrder = true;
  numTargetsInOrder = 0;

  for (byte count=0; count<numSwitches; count++) {
    switchArray[count] = 0xFF;
    bankBitmask *= 2;
    bankBitmask |= 1;
  }
  for (byte count=0; count<numSolenoids; count++) solArray[count] = 0xFF;

  targetResetTime = 0;
  ignoreDropsUntilTime = 0;
}

DropTargetBank::~DropTargetBank() {
  delete switchArray;
  delete solArray;
}

void DropTargetBank::DefineSwitch(byte switchOrder, byte switchNum) {
  if (switchOrder>=numSwitches) return;
  switchArray[switchOrder] = switchNum;
}

void DropTargetBank::AddAllTargetsSwitch(byte s_allTargetsSwitch) {
  if (bankType!=DROP_TARGET_TYPE_WLLMS_1) return;  
  allTargetsSwitch = s_allTargetsSwitch;
}

void DropTargetBank::DefineResetSolenoid(byte solIndex, byte solChannelNumber) {
  if (solIndex>=numSolenoids) return;
  solArray[solIndex] = solChannelNumber;
}


byte DropTargetBank::HandleDropTargetHit(byte switchNum) {
  byte oldStatus = bankStatus;

  byte targetBits = 0x00;
  byte singleBit = 0x01;

  if (ignoreDropsUntilTime) {
    return 0;
  }
  
  for (byte count=0; count<numSwitches; count++) {
    if (switchNum==switchArray[count]) {
      // If this is a new hit, see if it's in order
      if ((bankStatus & singleBit)==0x00) {
        if (targetsHitInOrder && count==numTargetsInOrder) {
          numTargetsInOrder += 1;        
        } else {
          targetsHitInOrder = false;
          numTargetsInOrder = 0;
        }
      }
      targetBits |= singleBit;
      break;
    }
    singleBit *= 2;
  }

  if (allTargetsSwitch!=0xFF) {
#ifdef BSOS_CONFIG_H    
    if (BSOS_ReadSingleSwitchState(allTargetsSwitch) || switchNum==allTargetsSwitch) targetBits = bankBitmask & (~bankStatus);
#elif defined WOS_CONFIG_H
    if (WOS_ReadSingleSwitchState(allTargetsSwitch) || switchNum==allTargetsSwitch) targetBits = bankBitmask & (~bankStatus);
#endif
  }

  bankStatus |= targetBits;
  if (bankStatus==bankBitmask) bankCleared = true;

  return bankStatus & ~oldStatus;
}

byte DropTargetBank::CheckIfBankCleared() {
  if (bankCleared) {
    if (targetsHitInOrder) return DROP_TARGET_BANK_CLEARED_IN_ORDER;
    else return DROP_TARGET_BANK_CLEARED;
  }   
  return 0;
}

byte DropTargetBank::GetStatus() {
  byte bitMask = 0x01;
  byte returnStatus = 0x00;
  for (byte count=0; count<numSwitches; count++) {
#ifdef BSOS_CONFIG_H    
    if (BSOS_ReadSingleSwitchState(switchArray[count])) returnStatus |= bitMask;
#elif defined WOS_CONFIG_H
    if (WOS_ReadSingleSwitchState(switchArray[count])) returnStatus |= bitMask;
#endif
    bitMask *= 2;
  }
  return returnStatus;
}


void DropTargetBank::ResetDropTargets(unsigned long timeToReset, boolean ignoreQuickDrops) {
  bankCleared = false;
  targetsHitInOrder = true;
  numTargetsInOrder = 0;

  if (numSolenoids) {
    for (byte count=0; count<numSolenoids; count++) {
#ifdef BSOS_CONFIG_H
      if (solArray[count]!=0xFF) BSOS_PushToTimedSolenoidStack(solArray[count], solenoidOnTime, timeToReset);
#elif defined WOS_CONFIG_H
      if (solArray[count]!=0xFF) WOS_PushToTimedSolenoidStack(solArray[count], solenoidOnTime, timeToReset);
#endif      
//      char buf[256];
//      sprintf(buf, "Resetting sol %d for %d cycles at %lu\n", solArray[count], solenoidOnTime, timeToReset);
//      Serial.write(buf);      
    }
    targetResetTime = timeToReset + 100; // This could be based on solenoidOnTime, but that's not currently set in ms
    if (ignoreQuickDrops) {
      ignoreDropsUntilTime = targetResetTime + 100;
    } else {
      ignoreDropsUntilTime = 0;
    }
  }

  return;
}

void DropTargetBank::Update(unsigned long currentTime) {
  if (targetResetTime && currentTime>targetResetTime) {
    bankStatus = GetStatus();
    targetResetTime = 0;
  }
  if (ignoreDropsUntilTime && currentTime>ignoreDropsUntilTime) {
    ignoreDropsUntilTime = 0;
  }
}
