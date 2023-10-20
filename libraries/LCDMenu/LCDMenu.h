#ifndef LCDMENU_H
#define LCDMENU_H


#include <Map.h>
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "LCDItem.h"

#define MAX_ITEM_NUMBER 5

class LCDMenu {
  public:
    LCDMenu(const LiquidCrystal_I2C& l);
    void moveUp();
    void moveDown();
    void select();
    void addHome(LCDItem* item);
    void goHome();
    void goBack();
    void update();
  private:
    void render();
    LCDItem* current;
    LiquidCrystal_I2C lcd;
};

#endif
