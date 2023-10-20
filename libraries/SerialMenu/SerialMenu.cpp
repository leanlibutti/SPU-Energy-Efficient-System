#include <Arduino.h>
#include "SerialMenu.h"

// Constructor
Menu::Menu(){
  numberOfEvents = 0;
  defaultSet = false;
}

// Se ejecuta automaticamente con cada loop, lee el serial
void Menu::serialEvent() {
  bool stringComplete = false;
  String inputString = "";
  inputString.reserve(50);
  String args[MAX_ARG_NUMBER];
  int argc = 0;
  for(int i = 0; i < MAX_ARG_NUMBER; i++){
    args[i] = "";
    args[i].reserve(10);
  }
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    if (inChar == '\n') {
      stringComplete = true;
      break;
    } else if(inChar == ','){
      argc++;
    } else if(!argc){
      inputString += inChar;
    } else {
      args[argc-1] += inChar;
    }
  }
  if (stringComplete){
    handleEvents(inputString, args, argc);
  }
}

void Menu::handleEvents(String message, String arguments[], int argc){
  for(int i = 0; i < numberOfEvents; i++){
    if(message.equalsIgnoreCase(events[i].name)){
      (*events[i].action)(argc, arguments);
      return;
    }
  }
  if(defaultSet) {
    String args[1] = {message};
    defaultEvent(1, args);
  }
}

bool Menu::addEvent(char* name, void (*action)(int argc, String args[])){
  if(numberOfEvents < MAX_EVENT_NUMBER){
    events[numberOfEvents].name = String(name);
    events[numberOfEvents].action = action;
    numberOfEvents++;
    return true;
  } else {
    return false;
  }
}

void Menu::removeEvent(char* name){
  for(int i = 0; i < numberOfEvents; i++){
    if(events[i].name.equalsIgnoreCase(name)){
      Serial.printf("\n DELETING %s", name);
      numberOfEvents--;
      for(int j = i; j < numberOfEvents; j++){
        events[j] = events[j+1];
      }
      return;
    }
  }
}

void Menu::printEvents(){
  Serial.printf("\n Menu events:");
  for(int i = 0; i < numberOfEvents; i++){
    Serial.printf("\n   %s", events[i].name.c_str());
  }
}

void Menu::addDefault(void (*action)(int argc, String args[])){
  defaultEvent = action;
  defaultSet = true;
}