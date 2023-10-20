#include <LEDIndicator.h>

LEDIndicator::LEDIndicator(int red_pin, int green_pin){
	redPin = red_pin;
  greenPin = green_pin;
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  turnOff();
}

void LEDIndicator::update(){
  if(isOn && (isBlinking || isAlternating) && ((millis() - stateChange) > BLINK_PERIOD)){
    stateChange = millis();
    //Serial.printf("\n LED UPDATE");
    if(isBlinking){
      if(state){
        if(isGreen){
          //Serial.printf("\n LED OFF GREEN");
          digitalWrite(greenPin, LOW);
        } else {
          //Serial.printf("\n LED OFF RED");
          digitalWrite(redPin, LOW);
        }
      } else {
        if(isGreen){
          //Serial.printf("\n LED ON GREEN");
          digitalWrite(greenPin, HIGH);
        } else {
          //Serial.printf("\n LED ON RED");
          digitalWrite(redPin, HIGH);
        }
      }
      state = !state;
    } else if(isAlternating){
      if(isGreen){
        //Serial.printf("\n LED ALTERNATE RED");
        digitalWrite(greenPin, LOW);
        digitalWrite(redPin, HIGH);
      } else {
        //Serial.printf("\n LED ALTERNATE GREEN");
        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);
      }
      isGreen = !isGreen;
    }
  }
}

void LEDIndicator::turnOff(){
  //Serial.printf("\n LED OFF");
  isOn = false;
  isAlternating = false;
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
}

void LEDIndicator::blinkRed(){
  //Serial.printf("\n LED BLINK RED");
  isOn = true;
  isGreen = false;
  isBlinking = true;
  stateChange = 0;
  state = false;
  isAlternating = false;
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
}

void LEDIndicator::blinkGreen(){
  //Serial.printf("\n LED BLINK GREEN");
  isOn = true;
  isGreen = true;
  isBlinking = true;
  stateChange = 0;
  state = false;
  isAlternating = false;
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
}

void LEDIndicator::stillRed(){
  //Serial.printf("\n LED RED");
  isOn = true;
  isGreen = false;
  isBlinking = false;
  isAlternating = false;
  digitalWrite(redPin, HIGH);
  digitalWrite(greenPin, LOW);
}

void LEDIndicator::stillGreen(){
  //Serial.printf("\n LED GREEN");
  isOn = true;
  isGreen = true;
  isBlinking = false;
  isAlternating = false;
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, HIGH);
}

void LEDIndicator::alternate(){
  //Serial.printf("\n LED ALTERNATE");
  isOn = true;
  isAlternating = true;
  isBlinking = false;
  stateChange = 0;
  isGreen = true;
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
}