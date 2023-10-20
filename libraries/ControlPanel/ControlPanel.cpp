#include <ControlPanel.h>

ControlPanel::ControlPanel(){
  callbackAmount = 0;
  client = WiFiClient();
  debug = millis();
  response.reserve(1000);
  message.reserve(CP_MAX_MESSAGE_SIZE);
  packet.reserve(1000);
  packet = "";
}

void ControlPanel::connect(char* newIp, int newPort){
  strncpy(ip, newIp, 19);
  port = newPort;
  if(!client.connect(ip, port)){
    Serial.printf("\n Couldnot connect.");
  }
}

void ControlPanel::update(){
  if(!client.connected() && (millis() - debug > 10000)){
    Serial.printf("\n Disconnected from server");
    client.connect(ip, port);
    debug = millis();
    return;
  }
  
  while (client.available()) {
    if(readPacket(packet)){
      int argc;
      message = "";
      initArguments(&argc, args);

      int pos = 0;
      while((packet[pos] != '&') && (packet[pos] != '\n')){
        message += packet[pos];
        pos++;
      }
      while((packet[pos] != '\n')){
        if(packet[pos] == '&'){
          if(argc == CP_MAX_ARGC){
            call(message, argc, args);
            initArguments(&argc, args);
          }
          argc++;
        } else {
          args[argc-1] += packet[pos];
        }
        pos++;
      }
      call(message, argc, args);
      initArguments(&argc, args);
      packet = "";
    }
  }
}

void ControlPanel::initArguments(int* argc, String argv[]){
  (*argc) = 0;
  for(int i = 0; i < CP_MAX_ARGC; i++)
    argv[i] = "";
}

bool ControlPanel::readPacket(String& packet){
  char inChar;
  while((inChar = (char)client.read()) != -1){
    yield();
    packet += inChar;
    if(inChar == '\n')
      return true;
  }
  return false;
}

void ControlPanel::call(String message, int argc, String args[]){
  int callback = getCallbackNumber(message);
  Serial.printf("\n Message: %s", message.c_str());
  for(int i = 0; i < argc; i++) Serial.printf("\n Arg %d: %s", i, args[i].c_str());
  if(callback == -1){
    Serial.printf("\n Unknown message.");
    return;
  }
  response = "";
  callbacks[callback].handler(argc, args);
  Serial.printf("\n Response: %s", response.c_str());
  if(response != ""){
    client.setNoDelay(true);
    response += "\n";
    int length = response.length() - 1 ;
    char l[5];
    snprintf(l, 5, "%04d", length);
    response = String(l) + response;
    client.print(response);
  }
}

int ControlPanel::getCallbackNumber(String message){
  for(int i = 0; i < callbackAmount; i++){
    if(callbacks[i].name == message){
      return i;
    }
  }
  return -1;
}

void ControlPanel::addToResponse(const char* r){
  response += r;
  response += '&';
}

void ControlPanel::addToResponse(int r){
  addToResponse(String(r).c_str());
}

void ControlPanel::createHandler(char* message, void (*handler)(int args, String argv[])){
  if(callbackAmount < MAX_CALLBACK_AMOUNT){
    callbacks[callbackAmount].name = String(message);
    callbacks[callbackAmount].handler = handler;
    callbackAmount++;
  }
}

void ControlPanel::send(const char* data){
  client.print(String(data)+"\n");
}