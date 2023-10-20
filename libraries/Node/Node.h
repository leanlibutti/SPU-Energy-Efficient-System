#ifndef NODE_H
#define NODE_H

#include <Arduino.h>
#include <cppQueue.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "SPU.h"

class Node {
public:
  Node(WiFiUDP& u, IPAddress ip);
  int getState();
  IPAddress getIp();
  void setIp(IPAddress ip);
  void sendMessage(int code);
  void sendPacket(int code, int packet[], int len);
  void clearMessages(); // Si hay mensajes pendientes de envio, los elimina
  void sendAck();
  void receiveAck();
  void checkSentPackets();
  void reconnect();
  void error();
  void handleSensorInfo(bool tempConnected, bool pirConnected, bool irConnected, relayInfo* r, int irGroup=0);
  void connectRelay(int relay, int group);
  void disconnectRelay(int relay);
  void disconnectFromGroup(int group);
  bool isRelayConnected(int relay);
  bool isRelayInGroup(int relay, int group);
  bool hasPirSensor();
  void connectPir();
  void disconnectPir();
  bool hasTemperatureSensor();
  void connectTemperatureSensor();
  void disconnectTemperatureSensor();
  void connectIrControl(int group);
  void disconnectIrControl();
  bool hasIrControl();
  bool hasCurrentSensor();
  bool hasPendingState();
  bool hasPendingResponse();
  bool hasErrorState();
  bool hasRunningState();
protected:
  // Estructura que representa un paquete a enviar
  typedef struct t_packet{
    int payload[MAX_PACKET_SIZE];
    int length;
  } Packet;

  IPAddress ip;
  int state; // Estado del nodo
  int lastMessageNumber; // Ultimo numero de mensaje enviado
  Queue pendingPackets; // Cola de mensajes del nodo
  int timeSinceSent; // Periodos de SENT_CHECK_TIME segundos desde que se envio el paquete y no hay respuesta
  bool pirSensorConnected; // Indican que sensores tiene conectado el nodo
  bool temperatureSensorConnected;
  bool currentSensorConnected;
  bool irControlConnected;
  int irControlGroup;
  WiFiUDP& udp; // Socket para enviar mensajes UDP
  relayInfo relays[MAX_SLAVE_RELAY_AMOUNT]; // Informacion de los relays del nodo, definido en SPU.h
  void sendNextPacket();
};

#endif
