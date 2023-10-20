#ifndef IRRECORD_H
#define IRRECORD_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <SPU.h>

class IRRecord {
public:
  IRRecord(int pin);
  bool record();
  int getValues(unsigned short v[]);
  bool codeRecorded();
private:
  IRrecv irrecv;
  decode_results results;
  int inputPin;
  int codeLength;
  unsigned short values[IR_CODE_LENGTH];
};

#endif