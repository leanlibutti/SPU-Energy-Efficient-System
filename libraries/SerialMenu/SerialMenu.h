#ifndef SERIAL_MENU_H
#define SERIAL_MENU_H

#include <Arduino.h>

#define MAX_EVENT_NUMBER 30
#define MAX_ARG_NUMBER 10

typedef struct {
  String name;
  void (*action)(int argc, String arg[]);
} event;

class Menu {
  public:
    Menu();
    void serialEvent();
    void handleEvents(String message, String arguments[], int argc);
    bool addEvent(char * name, void (*action)(int argc, String arg[]));
    void removeEvent(char* name);
    void printEvents();
    void addDefault(void (*action)(int argc, String arg[]));
  private:
    event events[MAX_EVENT_NUMBER];
    int numberOfEvents;
    bool defaultSet;
    void (*defaultEvent)(int argc, String arg[]);
};

#endif