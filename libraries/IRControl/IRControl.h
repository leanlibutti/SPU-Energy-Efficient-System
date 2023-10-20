#ifndef IRCONTROL_H
#define IRCONTROL_H

#include <Arduino.h>
#include <EEPROM.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRRecord.h>
#include <SPU.h>

class IRControl{
public:
  IRControl(int p);
  void sendCode();
  void storeCode(unsigned short v[], int size);
  void begin();
protected:
  IRsend irsend;
  bool state;
  int outputPin;
  int codeLength;
  uint16_t rawCode[IR_CODE_LENGTH];
};

#endif