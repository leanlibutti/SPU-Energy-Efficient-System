#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encoder {
  public:
    Encoder(int clk, int dt, int sw);
    void check();
    void onMove(void (*action)(bool direction));
    void onClick(void (*action)());
    void onHold(void (*action)());
  private:
    int CLK;  // Connected to CLK on KY-040
    int DT;  // Connected to DT on KY-040
    int SW;
    byte Last_DialPos;
    bool buttonPressed;
    int timeHold;
    void (*onMoveEvent)(bool direction); // Se ejecuta cuando se mueve el encoder, moveUp es true si se mueve hacia arriba
    void (*onClickEvent)(); // Se ejecuta al presionarse el boton
    void (*onHoldEvent)();
};

#endif