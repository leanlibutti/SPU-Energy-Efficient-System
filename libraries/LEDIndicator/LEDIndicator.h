#ifndef LEDINDICATOR_H
#define LEDINDICATOR_H

#include <Arduino.h>
#include <SPU.h>

#define BLINK_PERIOD 500

class LEDIndicator{
public:
  LEDIndicator(int red_pin, int green_pin);
  void blinkRed();
  void blinkGreen();
  void stillRed();
  void stillGreen();
  void alternate();
  void turnOff();
  void update();
protected:
  bool isGreen;
  bool isBlinking;
  bool isOn;
  bool state;
  bool isAlternating;
  int redPin;
  int greenPin;
  int stateChange;
};

#endif