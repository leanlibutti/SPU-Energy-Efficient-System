#include <Encoder.h>

void defaultOnMove(bool direction){};
void defaultOnClick(){};

Encoder::Encoder(int clk, int dt, int sw){
  pinMode(clk, INPUT);
  pinMode(dt, INPUT);
  pinMode(sw, INPUT);
  digitalWrite(SW, LOW);
  CLK = clk;
  DT  = dt;
  SW = sw;
  buttonPressed = false;
  timeHold = 0;
  onMove(defaultOnMove);
  onClick(defaultOnClick);
}

void Encoder::check(){
  byte DialPos = (digitalRead(CLK) << 1) | digitalRead(DT);
  
  /* Is the dial being turned anti-clockwise? */
  if (DialPos == 3 && Last_DialPos == 1){
    onMoveEvent(false);
  }
  
  /* Is the dial being turned clockwise? */
  if (DialPos == 3 && Last_DialPos == 2){
    onMoveEvent(true);
  }
  
  /* Output the counter to the serial port */
  //Serial.println(counter);
  
  /* Is the switch pressed? */
  if(digitalRead(SW)){
    if(!buttonPressed){
      buttonPressed = true;
    } else timeHold++;
    if(timeHold == 350){
      onHoldEvent();
    }
  } else {
    if(buttonPressed && (timeHold < 350)){
      onClickEvent();
    }
    buttonPressed = false;
    timeHold = 0;
  }
  
  /* Save the state of the encoder */
  Last_DialPos = DialPos;
}

void Encoder::onMove(void (*action)(bool direction)){
  onMoveEvent = action;
}

void Encoder::onClick(void (*action)()){
  onClickEvent = action;
}

void Encoder::onHold(void (*action)()){
  onHoldEvent = action;
}