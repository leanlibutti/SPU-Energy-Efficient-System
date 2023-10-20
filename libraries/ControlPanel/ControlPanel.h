#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#define CP_MAX_MESSAGE_SIZE 50
#define CP_MAX_ARGC 10
#define CP_MAX_ARG_SIZE 10

#define MAX_CALLBACK_AMOUNT 30

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <stdarg.h>
#include <string.h>

class ControlPanel {
public:
  ControlPanel();
  void connect(char* ip, int port);
  void update();
  void createHandler(char* message, void (*handler)(int args, String argv[]));
  void addToResponse(const char* response);
  void addToResponse(int response);
  void send(const char* data);
protected:
  typedef struct {
    String name;
    void (*handler)(int args, String argv[]);
  } callback;
  callback callbacks[MAX_CALLBACK_AMOUNT];
  int callbackAmount;
  String response;
  String packet;
  String args[CP_MAX_ARGC];
  String message;
  WiFiClient client;
  char ip[19]; // Tamaño maximo del string de ip
  int port;
  unsigned long debug;
  int getCallbackNumber(String message);
  void call(String message, int argc, String args[]);
  bool readPacket(String& packet);
  void initArguments(int* argc, String args[]);
};

#endif