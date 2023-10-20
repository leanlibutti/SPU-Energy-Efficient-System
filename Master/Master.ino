
#define _DISABLE_TLS_
#define _DEBUG_
#define THINGER_SERVER "192.168.100.1"

#include <Map.h>
#include <string.h>
#include <cppQueue.h>
#include <EEPROM.h>
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiUdp.h>
#include "compileTime.h"
#include <SPU.h>
#include <SerialMenu.h>
#include <Calendar.h>
#include <Node.h>
#include <Encoder.h>
#include <LCDMenu.h>
#include <ControlPanel.h>
#include <IRControl.h>
#include "DHT.h"
#include <SPI.h>
#include <ThingerWifi.h>

#define DHTTYPE DHT22
// Pines de perisfericos
#define ENCODER_A_PIN D5
#define ENCODER_B_PIN D0
#define ENCODER_BUTTON_PIN D6
#define RELAY1_PIN D3
#define RELAY2_PIN D4
#define PIR_PIN D8
#define DHT_PIN D8
#define IR_RECORD_PIN D7 // No definido

#define MAX_NODES 8 //Numero maximo de nodos conectados
#define SENT_CHECK_TIME 2 // Tiempo cada cuanto se checkean los nodos que nos respondieron

DHT dht(DHT_PIN, DHTTYPE); // Sensor de temperatura y humedad

// Caracter especial de temperatura, seria mejor moverlo a libreria LCD
byte customChar[] = {
  B11100,
  B10100,
  B11100,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

WiFiUDP udp; // Socket UDP
ControlPanel panel; // Panel de control, comunicacion con la aplicacion

char nodeName[NAME_LENGTH];

int connectedNodes = 0; // Numero de nodos conectados
Node* nodes[MAX_NODES]; // Nodos conectados
int baseIp = 4; // IP a partir de la cual otorgar IPs a nodos, 192.168.4.4

// Estructura que representa un grupo en el maestro
typedef struct t_group{
  t_group(): isOn(false), isConnected(false), name(""), isSleeping(false), ignorePir(false){};
  String name;
  bool isOn;        // Indica si los relays del grupo estan prendidos
  bool isConnected; // Indica si el grupo esta en uso
  bool isSleeping;  // Indica si el grupo esta 'dormido' por que no se detecta prensencia
                    // en la habitacion, en tal caso 'isOn' es true, para indicar que debe prenderse
                    // si se detecta prensencia
  bool ignorePir;
  int motionDetectionTime;
} relayGroup;

relayGroup relayGroups[GROUP_AMOUNT]; // Grupos de relays
relayInfo relays[MAX_MASTER_RELAY_AMOUNT];

// Estructura para almacenar info de sensores en el maestro
struct SensorInfo{
  struct PirInfo{
    PirInfo(): isConnected(false), isOn(false), motionDetected(false), motionDetectedOnSlave(false),
      isSleeping(false), isDisabled(false), inactivityTime(DEFAULT_MOTION_DET_TIME){};
    bool isConnected;       // Indica si el pir esta en uso
    bool isOn;              // Indica si se esta escuchando al pir en el momento (si hay grupo encendidos)
    bool motionDetected;    // Indica si se detecto movimiento en el maestro
    bool motionDetectedOnSlave;
    bool isSleeping;        // Indica si se apagaron los grupos por falta de movimiento
    bool isDisabled;        // Indica si se inhabilitó el pir temporalmente
    int detectionTime;      // Tiempo que permanecen prendidas las luces luego de que cese el movimiento
    int inactivityTime;     // se inhabilita por un tiempo configurable mientras se apagan los grupos
    int nextSleepingGroup;
  } pir;
  struct dhtInfo{
    dhtInfo(): isConnected(false), requestedFromPanel(false){};
    bool isConnected;       // Indica si el maestro tiene un sensor de temperatura y humedad conectado
    bool requestedFromPanel;
  } dht;
};

struct SensorInfo sensors;
bool lightsOn = false;             // Indica si las luces de la habitación se encuentran prendidas
int motionAlarm = ALARM_NOT_SET;   // Alarma para apagar las luces
int enablePirAlarm = ALARM_NOT_SET;// Alarma para habilitar el pir luego del tiempo de inactividad
int stateCheckAlarm = ALARM_NOT_SET;
int messageCheckAlarm = ALARM_NOT_SET;
int calendarCheckAlarm = ALARM_NOT_SET;
int currentMotionDetectionTime;    // Tiempo de deteccion de movimiento del proximo grupo en apagarse (solo para cuando se debe resetear la alarma)
int ambientReadAlarm = ALARM_NOT_SET;
int temperature = 0;
short int humidity = 0;
bool ambientRead = false;      // Indica si fue posible leer el sensor de temperatura correctamente
bool ambientDataSent = false;  // Indica si ya se envió información de temperatura (en caso de fallar la lectura, se  envía solo una vez)
int activeRelays = 0;
int activeIrControls = 0;
bool startPairingMode = false; // Flag que indica que se debe empezar a emparejar los nodos
int pairingTime = 60;          // Tiempo por defecto del emparejamiento
bool recordIRCode = false;     // Indica que se debe grabar un nuevo comando IR
bool resetFromPanel = false;
void startPairing(int seconds);
//void ICACHE_RAM_ATTR detectMotion();
int reconnectionTime = 0;

unsigned short irCode[IR_CODE_LENGTH];
int irCodeLength = 0;
bool irCodeComplete = false;

// Librerias de menu y alarmas
AlarmCalendar calendar;
Menu menu;
IRRecord irrecord(IR_RECORD_PIN);
LiquidCrystal_I2C lcd(0x3F, 20, 4);
LCDMenu visualMenu(lcd);
Encoder encoder(ENCODER_A_PIN, ENCODER_B_PIN, ENCODER_BUTTON_PIN);
ThingerWifi thing("iii_lidi", "prueba_placa", "123456");    

// ---- RTC ---- //
//#include "DS1302.h";

#include <RtcDS3231.h>
#define countof(a) (sizeof(a) / sizeof(a[0]))
RtcDS3231<TwoWire> Rtc(Wire);

// Inicializa el DS3231
void initRTC(){
  Rtc.Begin();
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  RtcDateTime h = Rtc.GetDateTime();
  setTime(h.Hour(), h.Minute(), h.Second(), h.Day(), h.Month(), h.Year());
  //setTime(13, 9, 0, 27, 2, 19);
}

// Realiza tareas necesarias cada 5 segundos
// Controla alarmas del calendario, actualiza la hora y el menu LCD
void checkCalendarAlarms(){
  RtcDateTime now = Rtc.GetDateTime();
  setTime(now.Hour(), now.Minute(), now.Second(), now.Day(), now.Month(), now.Year());
  printActualTime();
  if(now.Second() < 5){
    visualMenu.update();
  }
  calendar.checkAlarms();
}

// Imprime fecha y hora actuales
void printActualTime() {
  // ---- RTC3231 ---- //
  //RtcDateTime a = Rtc.GetDateTime(); // save date
  //printDateTime(a);
  time_t t = now();
  Serial.printf("\n %d:%d:%d ", hour(t), minute(t), second(t));
}

/*void printDateTime(const RtcDateTime& dt){
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Day(),
            dt.Month(),            
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.printf("\n");
    Serial.print(datestring);
}*/

// Evento para el movimiento del encoder, en este caso el evento se notifica al menu
void encoderMove(bool isUp){
  if(isUp){
    visualMenu.moveUp();
  } else {
    visualMenu.moveDown();
  }
}

// Evento para el click del encoder
void encoderClick(){
  visualMenu.select();
}

// Evento para el click y mantener del encoder
void encoderHold(){
  visualMenu.goBack();
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(MEMORY_SIZE);

  udp.begin(DEFAULT_PORT);
  for (int i = 0; i < 50; i++) Serial.println();

  baseIp = getBaseIp(); //Lee la memoria EEPROM en busca de la siguiente IP libre
  
  encoder.onMove(encoderMove);  // Asigna los eventos del encoder
  encoder.onClick(encoderClick);
  encoder.onHold(encoderHold);
  initCalendar();
  initRelayGroups();
  initVisualMenu();
  initMasterSensors();
  initControlPanel();
  initThinger();
  initWifi();
  initRTC();
  initMenu();
  
  stateCheckAlarm = Alarm.timerRepeat(STATE_CHECK_TIME, checkNodeStatus); // Alarma para controlar el estado de los nodos
  messageCheckAlarm = Alarm.timerRepeat(SENT_CHECK_TIME, checkSentPackets); // Alarma para controlar el envio de paquetes
}

void loop() {
  int packetSize = udp.parsePacket(); 
  if(packetSize) { //Espera un paquete UDP
    handlePacket();
  }
  if(startPairingMode){
    startPairing(pairingTime);
  }
  if(resetFromPanel){
    Serial.printf("\n Reset desde panel.");
    delay(1000);
    ESP.reset();
  }
  menu.serialEvent();
  Alarm.delay(0);
  encoder.check();
  detectMotion();
  checkPir();
  checkIRCode();
  panel.update();
  checkConnection();
}

// Recibe el paquete y decide que hacer
void handlePacket() {
  Node* node;
  byte packet[MAX_PACKET_SIZE];
  int length = udp.read(packet, MAX_PACKET_SIZE);
  if ( (node = getNode(udp.remoteIP())) ) {                          // Si el nodo ya esta registrado
    if(length == 2){                                                 // Si la longitud es mayor a 2 bytes se trata de un paquete
      int message = (int)packet[0];
      handleMessage(message, node);
    } else  {
      handleData(packet, node);
    }
  } else if((int)packet[0] == CONNECTION_MSG){
    printActualTime();                                                // Si es un nodo nuevo
    Serial.printf(" New node: %s", udp.remoteIP().toString().c_str());
    if (connectedNodes < MAX_NODES) {                                 // Se crea una instancia para el nodo
      nodes[connectedNodes] = new Node(udp, udp.remoteIP());
      nodes[connectedNodes]->sendAck();
      connectedNodes++;
      visualMenu.update();
    } else
      Serial.printf("Maximun number of nodes reached.");
  }
}

// Maneja los mensajes de nodos conectados
void handleMessage(int message, Node* node) {
  printActualTime();
  if(message == CONNECTION_MSG){ // Si un nodo conectado vuelve a enviar mensaje de conexion, sabemos que se reinició
    Serial.printf("\nNode %s has been reset.", node->getIp().toString().c_str());
    node->reconnect();
    visualMenu.update();
    return;
  }
  if(message == IP_MSG){ // Si un nodo requiere IP nueva, se arma y envia. MODULARIZAR
    sendNewIP(node);
    return;
  }
  switch(node->getState()){ // Decide dependiendo del estado. Probablemente podria simplificarse
                            // y recibir todo en un solo switch
    case PENDING_STATE: // Se espera el estado del nodo
      switch(message){
        case RUNNING_STATE:
          Serial.printf(" %s node is running.", node->getIp().toString().c_str());
          node->receiveAck();
          break;
        case ERROR_STATE:
          Serial.printf(" ERROR IN NODE %s", node->getIp().toString().c_str());
          node->error();
          break;
        case MOTION_DETECTED_MSG: detectSlaveMotion(); // Tal vez deberían cambiar el estado a RUNNING??
      }
      visualMenu.update();
      node->sendAck();
      break;
    case PENDING_RESPONSE_STATE: // Se espera la respuesta de un mensaje
      if(message == RECEIVED_MSG || message == RUNNING_STATE){
        Serial.printf(" Node %d responded", nodeId(node));
        node->receiveAck();
        visualMenu.update();
      }
      break;
    case RUNNING_STATE: // Si el nodo esta corriendo, se esperan los mensajes de sensores etc
      switch (message) {
        case MOTION_DETECTED_MSG:
          node->sendAck();
          detectSlaveMotion();
          break;
        case RECEIVED_MSG:
          Serial.printf("\n Unexpected ACK");
          node->sendAck();
          break;
        default:
          Serial.printf(" Unknown message from node %s", node->getIp().toString().c_str());
          node->sendAck();
          break;
      }
      break;
  }
}

// Maneja los paquetes de datos de los nodos
void handleData(byte packet[], Node * node){
  int message = (int)packet[0]; // Indica que informacion contiene el paquete
  int position = 1; // Siguiente posicion que leer del ṕaquete
  switch(message){
    case SENSOR_DATA_MSG:
      // Se reciben datos de los sensores
      handleSensorData(packet, position, node);
      break;
    case SENSOR_INFO_MSG:
      // Se recibe la informacion de que sensores y relays un nodo tiene conectado
      Serial.printf("\nSensor info received from node %d", nodeId(node));
      handleSensorInfo(packet, node);
      break;
    default:
      Serial.printf("\nUnknown packet from node %s", node->getIp().toString().c_str());
      break;
  }
  node->sendAck();
  node->receiveAck();
}


// Resetea todas las alarmas que estuviesen funcionando
// Se usa cuando hay que corregir la hora
void resetAlarms(){
  if(motionAlarm != ALARM_NOT_SET){
    Alarm.free(motionAlarm);
    motionAlarm = Alarm.timerOnce(currentMotionDetectionTime, sleepGroups);
  }
  if(enablePirAlarm != ALARM_NOT_SET){
    Alarm.free(enablePirAlarm);
    enablePirAlarm = Alarm.timerOnce(sensors.pir.inactivityTime, enablePir);
  }
  if(stateCheckAlarm != ALARM_NOT_SET){
    Alarm.free(stateCheckAlarm);
    stateCheckAlarm = Alarm.timerRepeat(STATE_CHECK_TIME, checkNodeStatus);
  }
  if(messageCheckAlarm != ALARM_NOT_SET){
    Alarm.free(messageCheckAlarm);
    messageCheckAlarm = Alarm.timerRepeat(SENT_CHECK_TIME, checkSentPackets);
  }
  if(calendarCheckAlarm != ALARM_NOT_SET){
    Alarm.free(calendarCheckAlarm);
    calendarCheckAlarm = Alarm.timerRepeat(5, checkCalendarAlarms);
  }
}

void initThinger(){
  thing["groups_state"] >> [](pson& out){
    for(int i = 0; i < GROUP_AMOUNT; i++){
      if(relayGroups[i].isConnected){
        out[relayGroups[i].name.c_str()] = (relayGroups[i].isOn)?((relayGroups[i].isSleeping)?1:2):0;
      }
    }
  };

  thing["groups_state_text"] >> [](pson& out){
    for(int i = 0; i < GROUP_AMOUNT; i++){
      if(relayGroups[i].isConnected){
        out[relayGroups[i].name.c_str()] = (relayGroups[i].isOn)?((relayGroups[i].isSleeping)?"Inactivo":"Encendido"):"Apagado";
      }
    }
  };

  thing["complete_groups_state"] >> [](pson& out){
    time_t t = now();
    char time[50];
    snprintf(time, 50, "%d:%d:%d %d/%d/%d", hour(t), minute(t), second(t), day(t), month(t), year(t));
    out["stamp"] = String(time);
    for(int i = 0; i < GROUP_AMOUNT; i++){
      if(relayGroups[i].isConnected){
        out[relayGroups[i].name.c_str()] = (relayGroups[i].isOn)?((relayGroups[i].isSleeping)?1:2):0;
      }
    }
    out["node_name"] = String(nodeName);
  };

  thing["ambient_sensor"] >> [](pson& out){
    out["node"] = String(nodeName);
    out["success"] = ambientRead;
    out["temperature"] = temperature;
    out["humidity"] = humidity;
  };

  thing["complete_ambient_sensor"] >> [](pson& out){
    time_t t = now();
    char time[50];
    snprintf(time, 50, "%d:%d:%d %d/%d/%d", hour(t), minute(t), second(t), day(t), month(t), year(t));
    out["stamp"] = String(time);
    out["node_name"] = String(nodeName);
    out["success"] = ambientRead;
    out["temperature"] = temperature;
    out["humidity"] = humidity;
  };

  thing["complete_actuator_info"] >> [](pson& out){
    time_t t = now();
    char time[50];
    snprintf(time, 50, "%d:%d:%d %d/%d/%d", hour(t), minute(t), second(t), day(t), month(t), year(t));
    out["relays"] = activeRelays;
    out["ir"] = activeIrControls;
    out["node_name"] = String(nodeName);
  };

  thing["actuator_info"] >> [](pson& out){
    int activeRelays, activeIrControls;
    activeActuators(&activeRelays, &activeIrControls);
    out["relays"] = activeRelays;
    out["ir"] = activeIrControls;
    out["node"] = String(nodeName);
  };
}

// Pide el estado de todos los nodos, cada 30 segundos
// Ignorar si se esta esperando una respuesta del nodo?
void checkNodeStatus() {
  int i ;
  for (i = 0; i < connectedNodes; i++) {
    if(nodes[i]->hasRunningState() || nodes[i]->hasPendingState()){
      nodes[i]->sendMessage(STATE_MSG);
      Serial.printf("\n Checking status for %s", nodes[i]->getIp().toString().c_str());
    } 
  }
  visualMenu.update();
}

// Reenvia los paquetes sin respuesta a los nodos
void checkSentPackets(){
  Node* node;
  for(int i = 0; i < connectedNodes; i++){
    nodes[i]->checkSentPackets();
  }
}

// Envia broadcast, se espera respuesta de todos los nodos
void sendGlobalMessage(int code) {
  for (int i = 0; i < connectedNodes; i++) {
    nodes[i]->sendMessage(code);
  }
}

// Envia broadcast de paquete, se espera respuesta de todos los nodos
void sendGlobalPacket(int message, int packet[], int len) {
  for (int i = 0; i < connectedNodes; i++) {
    nodes[i]->sendPacket(message, packet, len);
  }
}

// Devuelve el nodo con el ip que se pasa, o 0 si no es un nodo conectado
Node * getNode(IPAddress ip) {
  for (int i = 0; i < connectedNodes; i++) {
    if (nodes[i]->getIp() == ip) {
      return nodes[i];
    }
  }
  return 0;
}

Node * getNode(int ip) {
  for (int i = 0; i < connectedNodes; i++) {
    if (nodes[i]->getIp()[3] == ip) {
      return nodes[i];
    }
  }
  return 0;
}

// COntrola que haya un pir conectado en maestro o cualquiera de los nodos
bool connectedPir(){
  if(sensors.pir.isConnected) return true;
  for(int i = 0; i < connectedNodes; i++){
    if(nodes[i]->hasPirSensor())
      return true;
  }
  return false;
}

// Calcula la ultima ip que se otorgó a un nodo
// Seria mejor guardar solo la ultima direccion utilizada
int getBaseIp(){
  int ip, base= baseIp;
  for(int i = 0; i < MAX_IP_AMOUNT; i++){
    ip = EEPROM.read(i+SLAVE_IP_DIR);
    if(ip > base) base= ip;
  }
  return base;
}

// Almacena nueva ip
void storeIp(IPAddress ip){
  EEPROM.write(baseIp-4+SLAVE_IP_DIR, ip[3]);
  EEPROM.commit();
}

// Genera la siguiente IP libre para darsela a un nodo
void sendNewIP(Node* node){
  baseIp++;                              // La baseIp es la ultima utilizada, se le suma uno
  IPAddress newIp(192, 168, 4, baseIp);  // se le agrega el resto de la direccion
  int *ip= (int*)malloc(sizeof(int)*4);  // se arma el paquete
  for(int i= 0; i < 4; i++){
    ip[i]= newIp[i];
  }
  node->sendPacket(IP_MSG, ip, 4);       // Se envia y se almacena localmente
  node->setIp(newIp);
  free(ip);
  storeIp(newIp);                        // Se guarda como reservada en EEPROM
  Serial.printf("\nNew IP sent: %s", node->getIp().toString().c_str());
}

// DEBUG: imprime contenido de la EEPROM
void printEprom(int argc, String args[]){
  for(int i = 0; i < 71; i++){
    Serial.printf("\nPosition %d: %d", i, EEPROM.read(i));  
  }  
}

// Devuelve el numero de nodo
int nodeId(Node * node){
  for(int i = 0; i < connectedNodes; i++){
    if(node->getIp() == nodes[i]->getIp()) return i;
  }
  return -1;
}

// Devuelve el numero de nodo a partir del ultimo numero de la IP
int nodeFromIp(int ip){
  for(int i = 0; i < connectedNodes; i++){
    if(nodes[i]->getIp()[3] == ip) return i;
  }
  return -1;
}

// Recibe el paquete con la informacion de los sensores y relays del esclavo y la almacena
void handleSensorInfo(byte packet[], Node* node){
  bool b[8];
  bool groups[GROUP_AMOUNT];
  for(int i = 0; i < GROUP_AMOUNT; i++) groups[i] = false;
  int val = packet[1];
  for(int i= 7; i >= 0; i--){
    b[i]= (val % 2 == 1);
    val/=2;
  }
  relayInfo relays[MAX_SLAVE_RELAY_AMOUNT];
  for(int i= 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(b[i+RELAY_DIR]){
      relays[i].connected = true;
      relays[i].group = packet[i+2];
      groups[packet[i+2]] = true;
    }
  }
  if(b[IR_DIR]){   // Si hay actuador IR se almacena a que grupo pertenece
    int irGroup = packet[MAX_SLAVE_RELAY_AMOUNT+2];
    groups[irGroup] = true;
    node->handleSensorInfo(b[TEMP_DIR], b[PIR_DIR], b[IR_DIR], relays, irGroup);
  } else
    node->handleSensorInfo(b[TEMP_DIR], b[PIR_DIR], b[IR_DIR], relays);
  for(int i = 0; i < GROUP_AMOUNT; i++){
    if(groups[i]){
      int g[] = {i};
      if((relayGroups[i].isOn) && (!relayGroups[i].isSleeping))
        node->sendPacket(TURN_ON_GROUP_MSG, g, 1);
      else
        node->sendPacket(TURN_OFF_GROUP_MSG, g, 1);
    }
  }
}

// Recibe el paquete con datos de sensores y lo interpreta
void handleSensorData(byte packet[], int position, Node* node){
  // Formato del paquete
  // 1 byte de temperatura, se castea directamente a int
  // 2 bytes de corriente, se lee en short int, se divide por 1000 y castea a float
  if(node->hasTemperatureSensor()){                      // Datos del DHT22
    temperature = (int)packet[position] ;
    Serial.printf("\n Temperature: %d", temperature);
    position++;
    short int h = (short int)packet[position];
    humidity = h;
    position++;
    Serial.printf("\n Humidity: %d", h);
    if(sensors.dht.requestedFromPanel){
      char data[10];
      snprintf(data, 10, "%d&%d", temperature, humidity);
      panel.send(data);
      sensors.dht.requestedFromPanel = false;
    }
  } 
  if(node->hasCurrentSensor()){                          // Datos del sensor de corriente (ya no se usa)
    unsigned short int c = (unsigned short int)packet[position] << 8;
    position++;
    c += (unsigned short int)packet[position];
    position++;
    float current = (static_cast<float>(c))/1000;
    Serial.printf("\n Current: %f", current);
  }
  visualMenu.update();  // Se actualiza el menu para mostrar la informacion
}

// Lee la EEPROM y configura los sensores y relays conectados al maestro
void initMasterSensors(){
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  //pinMode(DHT_PIN, INPUT);  Es el mismo que PIR_PIN
  pinMode(PIR_PIN, INPUT);
  sensors.pir.isConnected = false;
  sensors.dht.isConnected = false; 
  relays[0].pin = RELAY1_PIN; 
  relays[1].pin = RELAY2_PIN;
  // Lee un byte, donde cada bit indica si el sensor o relay correspondiente esta conectado
  bool b[8];
  int val = EEPROM.read(SENSOR_INFO_DIR);
  for(int i= 7; i >= 0; i--){
    b[i]= (val % 2 == 1);
    val/=2;
  }
  if(b[PIR_DIR]){
    sensors.pir.isConnected = true;
    sensors.pir.isOn = false;
    sensors.pir.motionDetected = false;
  }
  if(b[TEMP_DIR]){
    sensors.dht.isConnected = true;
    ambientReadAlarm = Alarm.timerRepeat(10, sendSensorData);
  }
  byte pir_info[4];
  for(int i= 0; i < 4; i++) pir_info[i] = EEPROM.read(PIR_TIME_DIR+i); // Lee el tiempo de inactividad del pir
  memcpy(&sensors.pir.inactivityTime, &pir_info, sizeof(pir_info));
  for(int i = 0; i < MAX_MASTER_RELAY_AMOUNT; i++){
    int group = EEPROM.read(RELAY_MODE_DIR+i);
    if(group != 255){
      relays[i].connected = true;
      relays[i].on = false;
      relays[i].group = group;
    } else {
      relays[i].connected = false;
    }
  }
}

void storeSensorInfo(){
  bool b[8];
  for(int i = 0; i < 8; i++) b[i]= false;
  if(sensors.pir.isConnected) b[PIR_DIR] = true;
  if(sensors.dht.isConnected) b[TEMP_DIR] = true;
  int val = 0;
  for(int j= 0; j < 8; j++){
    val*=2;
    val+= (b[j])?1:0;
  }
  EEPROM.write(SENSOR_INFO_DIR, val);
  byte pir_info[4];
  memcpy(&pir_info, &sensors.pir.inactivityTime, sizeof(sensors.pir.inactivityTime));
  for(int i= 0; i < 4; i++) EEPROM.write(PIR_TIME_DIR+i, pir_info[i]);
  for(int i = 0; i < MAX_MASTER_RELAY_AMOUNT; i++){
    if(relays[i].connected)
      EEPROM.write(RELAY_MODE_DIR+i, relays[i].group);
    else
      EEPROM.write(RELAY_MODE_DIR+i, 255);
  }

  EEPROM.commit();
}

// Trata de leer la información del dht y enviarla a thinger.io
// Si no puede leer datos, notifica una vez al servidor
void sendSensorData(){
  ambientRead = getSensorData(&temperature, &humidity);
  if(ambientRead /*|| !ambientDataSent*/){
    thing.stream(thing["ambient_sensor"]);
    thing.write_bucket("GeneralAmbientData", "ambient_sensor");
    //if(ambientRead)
      thing.call_endpoint("NodeRedAmbientData", thing["complete_ambient_sensor"]);
    ambientDataSent = !ambientRead;
  }
}

// Prende el pir para empezar a detectar presencia a traves de interrupciones
// tambien indica que se debe escuchar pirs de esclavos
void turnOnPir(){
  if(sensors.pir.isOn) return;
  Serial.printf("\n Turning on pir");
  sensors.pir.isOn = true;
  if(sensors.pir.isConnected){
    sensors.pir.motionDetected = (digitalRead(PIR_PIN) == HIGH);
    //attachInterrupt(digitalPinToInterrupt(PIR_PIN), detectMotion, FALLING);
  }
}

// Apaga el pir
void turnOffPir(){
  for(int i = 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected && relayGroups[i].isOn && (!relayGroups[i].ignorePir))
      return;
  }
  Serial.printf("\n Turning off pir");
  sensors.pir.isOn = false;
  sensors.pir.isSleeping = false;
  if(motionAlarm != DISCONNECTED){
    Alarm.free(motionAlarm);
    motionAlarm = ALARM_NOT_SET;
  }
  //if(sensors.pir.isConnected)
  //  detachInterrupt(PIR_PIN);
}

// Evento que indica que se detectó movimiento en algun pir
void detectMotion(){
  if(!sensors.pir.isDisabled && sensors.pir.isConnected && sensors.pir.isOn){
    sensors.pir.motionDetected = (digitalRead(PIR_PIN) == HIGH);
    //Serial.printf("\n -----------------");
  }
}

void detectSlaveMotion(){
  if(!sensors.pir.isDisabled){
    sensors.pir.motionDetectedOnSlave = true;
  }
}


// Controla la deteccion de movimiento del pir
// Despierta los grupos si estaban dormidos
// Renueva la alarma para dormirlos si no se vuelve a detectar movimiento
void checkPir(){
  if(sensors.pir.isOn && (sensors.pir.motionDetected || sensors.pir.motionDetectedOnSlave) && connectedPir()){
    if(sensors.pir.motionDetected){
      Serial.printf("\n Motion detected on master");
      sensors.pir.motionDetected = false;
    }
    else {
      Serial.printf("\n Motion detected");
      sensors.pir.motionDetectedOnSlave = false;
    }
    if(motionAlarm != DISCONNECTED){
      Alarm.free(motionAlarm);
      motionAlarm = ALARM_NOT_SET;
    }
    if(sensors.pir.isSleeping){
      sensors.pir.isSleeping = false;
      for(int i = 0; i < GROUP_AMOUNT; i++){
        if(relayGroups[i].isConnected && relayGroups[i].isSleeping){
          relayGroups[i].isSleeping = false;
          if(relayGroups[i].isOn){
            Serial.printf("\n Despertando grupo %d", i);
            turnOnGroup(i);
          }
        }
      }
    }
    sensors.pir.nextSleepingGroup = getNextSleepingGroup();
    motionAlarm = Alarm.timerOnce(relayGroups[sensors.pir.nextSleepingGroup].motionDetectionTime, sleepGroups);
    currentMotionDetectionTime = relayGroups[sensors.pir.nextSleepingGroup].motionDetectionTime;
  }
}

int getNextSleepingGroup(){
  int min = 65535; // Maximo tiempo de espera para el pir
  int group = -1;
  for(int i = 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected && relayGroups[i].isOn && (!relayGroups[i].isSleeping) && (!relayGroups[i].ignorePir)){
      if(relayGroups[i].motionDetectionTime <= min){
        min = relayGroups[i].motionDetectionTime;
        group = i;
      }
    }
  }
  return group;
}

// Duerme los grupos hasta que se detecte movimiento
// Inhabilita el pir para que no detecte movimiento mientras los grupos se apagan
void sleepGroups(){
  motionAlarm = ALARM_NOT_SET;
  sensors.pir.isSleeping = true;
  int oldGroup = sensors.pir.nextSleepingGroup;
  if(relayGroups[oldGroup].isOn){
    Serial.printf("\n Durmiendo grupo %d", oldGroup);
    sleepGroup(oldGroup);
  }
  int newGroup = getNextSleepingGroup();
  while((newGroup != -1) && (relayGroups[newGroup].motionDetectionTime <= relayGroups[oldGroup].motionDetectionTime)){
    Serial.printf("\n Durmiendo grupo %d", newGroup);
    sleepGroup(newGroup);
    newGroup = getNextSleepingGroup();
  }
  if(newGroup != -1){
    sensors.pir.nextSleepingGroup = newGroup;
    motionAlarm = Alarm.timerOnce(relayGroups[newGroup].motionDetectionTime - relayGroups[oldGroup].motionDetectionTime, sleepGroups);
    currentMotionDetectionTime = relayGroups[newGroup].motionDetectionTime - relayGroups[oldGroup].motionDetectionTime;
  }
  if(sensors.pir.inactivityTime){
    enablePirAlarm = Alarm.timerOnce(sensors.pir.inactivityTime, enablePir);
    sensors.pir.isDisabled = true;
  }
}

// Habilita el pir
void enablePir(){
  sensors.pir.isDisabled = false;
  enablePirAlarm = ALARM_NOT_SET;
}

void connectMasterDht(){
  sensors.dht.isConnected = true;
  ambientReadAlarm = Alarm.timerRepeat(10, sendSensorData);
  storeSensorInfo();
}

void disconnectMasterDht(){
  sensors.dht.isConnected = false;
  storeSensorInfo();
}

void connectMasterPir(){
  sensors.pir.isConnected = true;
  for(int i = 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected && relayGroups[i].isOn){
      turnOnPir();
      break;
    }
  }
  storeSensorInfo();
}

void disconnectMasterPir(){
  turnOffPir();
  sensors.pir.isConnected = false;
  if(!connectedPir()){
    for(int i = 0; i < GROUP_AMOUNT; i++){
      if(relayGroups[i].isConnected && relayGroups[i].isOn && relayGroups[i].isSleeping){
        turnOnGroup(i);
      }
    }
  }
  storeSensorInfo();
}

// Registra las acciones programables
void initCalendar(){
  calendar = AlarmCalendar();
  calendar.addEvent(1, turnOnEverythingEvent, turnOffEverythingEvent);
  //calendar.addEvent(2, turnOnRelay, turnOffRelay);
  calendar.addEvent(3, turnOnGroupEvent, turnOffGroupEvent);
  calendarCheckAlarm = Alarm.timerRepeat(5, checkCalendarAlarms);
}

void turnOnEverythingEvent(int id){ 
  Serial.printf("\n Turn on everything");
  for(int i=0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      turnOnGroup(i);
    }
  }
  turnOnPir();
}

void turnOffEverythingEvent(int id){ 
  Serial.printf("\n Turn off everything");
  for(int i=0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      turnOffGroup(i, false);
    }
  }
  turnOffPir();
}

void turnOnGroupEvent(int id){ 
  Serial.printf("\n Turn on group %d", id);
  turnOnGroup(id);
  if(!relayGroups[id].ignorePir){
    turnOnPir();
    if(motionAlarm == ALARM_NOT_SET){
      motionAlarm = Alarm.timerOnce(relayGroups[id].motionDetectionTime, sleepGroups);
      currentMotionDetectionTime = relayGroups[id].motionDetectionTime;
    }
  }
}

void turnOffGroupEvent(int id){ 
  Serial.printf("\n Turn off group %d", id);
  turnOffGroup(id, false);
  if(!relayGroups[id].ignorePir)
    turnOffPir();
}

/*void turnOnRelay(int id){
  int nodeId = id / 10;
  int relayId = id % 10;
  if(nodeId == 0){
    if(relayId == 1) digitalWrite(RELAY1_PIN, LOW);
    if(relayId == 2) digitalWrite(RELAY2_PIN, LOW);
  } else {
    for(int i = 0; i < connectedNodes; i++){
      if(nodes[i].ip[3] == nodeId){
        Serial.printf("\n PRENDER RELAY %d de nodo %d", relayId, nodeId);
        break;
      }
    }
  }
}

void turnOffRelay(int id){
  int nodeId = id / 10;
  int relayId = id % 10;
  if(nodeId == 0){
    if(relayId == 1) digitalWrite(RELAY1_PIN, HIGH);
    if(relayId == 2) digitalWrite(RELAY2_PIN, HIGH);
  } else {
    for(int i = 0; i < connectedNodes; i++){
      if(nodes[i].ip[3] == nodeId){
        Serial.printf("\n APAGAR RELAY %d de nodo %d", relayId, nodeId);
        break;
      }
    }
  }
}*/

int relayNumber(Node* node, int r){
  return node->getIp()[3]*10 + r;
}


// Crea el menu serial
void initMenu(){
  menu.addEvent("EVENTS", printEvents);
  //menu.addEvent("DATA", getSensorData);
  menu.addEvent("CONNECT_PIR", connectPir);
  menu.addEvent("DISCONNECT_PIR", disconnectPir);
  menu.addEvent("CONNECT_TEMP", connectTemp);
  menu.addEvent("DISCONNECT_TEMP", disconnectTemp);
  //menu.addEvent("CONNECT_CURRENT", connectCurrent);
  //menu.addEvent("DISCONNECT_CURRENT", disconnectCurrent);
  /*menu.addEvent("CONNECT_RELAY1", connectRelay1);
  menu.addEvent("DISCONNECT_RELAY1", disconnectRelay1);
  menu.addEvent("CONNECT_RELAY2", connectRelay2);
  menu.addEvent("DISCONNECT_RELAY2", disconnectRelay2);*/
  menu.addEvent("NODES", printNodeInfo);
  menu.addEvent("CLEAR_EEPROM", clearEEPROM);
  menu.addEvent("PRINT_EEPROM", printEprom);
  menu.addEvent("TIME", setRTCTime);
  menu.addEvent("ALARMS", printAlarms);
  //menu.addEvent("weekly_alarm", addWeeklyAlarm);
  menu.addEvent("remove_alarm", removeAlarm);
  menu.addEvent("alarm_once", addAlarmOnce2);
  menu.addEvent("wifi_settings", changeWifi);
  menu.addEvent("wifi_ssid", changeSSID);
  menu.addEvent("wifi_passwd", changePassword);
  menu.addEvent("reset", reset);
  menu.addEvent("heap", showHeap);
  menu.addEvent("extwifi", serialExternalWifi);
  menu.addEvent("groups", serialGroupInfo);
  menu.addEvent("record_ir", panelRecordIR);
  menu.addEvent("transmit_ir", panelTransmitIrCode);
  menu.addEvent("pair", panelStartPairing);
  menu.addDefault(defaultEvent);
}

void serialGroupInfo(int argc, String argv[]){
  for(int i = 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      Serial.printf("\n %d: %s, State: %d", i+1, relayGroups[i].name.c_str(), relayGroups[i].isOn);
      if(relayGroups[i].ignorePir)
        Serial.printf(", ignore pir");
      else
        Serial.printf(", pir time: %d, Sleeping: %d", relayGroups[i].motionDetectionTime, relayGroups[i].isSleeping);
    }
  }
}

void serialExternalWifi(int argc, String args[]){
  if(argc != 3){
    Serial.printf("\n Ingrese SSID, contrasenia e IP.");
  } else {
    changeExternalWifiInfo(args[0], args[1], args[2]);
  }
}

void showHeap(int argc, String args[]){
  Serial.printf("\n Free heap: %d", ESP.getFreeHeap());
}

void reset(int argc, String args[]){
  ESP.reset();
}

void printAlarms(int argc, String args[]){
  calendar.showAlarms();
}

void addWeeklyAlarm(int argc, String args[], int id = -1){
  if(argc != 5){
    Serial.printf("\n New weekly alarm\n Enter day of the week, start hour, start minute,\n finish hour, finish minute and event code\n.");
  } else {
    int day = args[0].toInt(),
      startHour = args[1].toInt(),
      startMinute = args[2].toInt(),
      finHour = args[3].toInt(),
      finMinute = args[4].toInt(),
      type = (id == -1)?1:3;
    calendar.addWeeklyAlarm(day, startHour, startMinute, finHour, finMinute, type, id);
  }
}

void addAlarmOnce2(int argc, String args[]){
  if(argc < 8){
    Serial.printf("\n New alarm\n Enter day, month, year, start hour, start minute, finish hour, finish minute.");
  } else {
    int id = -1;
    int day = args[0].toInt(),
      mth = args[1].toInt(),
      yr = args[2].toInt(),
      startHour = args[3].toInt(),
      startMinute = args[4].toInt(),
      finHour = args[5].toInt(),
      finMinute = args[6].toInt(),
      type = (id == -1)?1:3;
    calendar.addAlarmOnce(day, mth, yr, startHour, startMinute, finHour, finMinute, type, id);
  }
}

void addAlarmOnce(int argc, String args[], int id = -1){
  if(argc < 7){
    Serial.printf("\n New alarm\n Enter day, month, year, start hour, start minute, finish hour, finish minute.");
  } else {
    int day = args[0].toInt(),
      mth = args[1].toInt(),
      yr = args[2].toInt(),
      startHour = args[3].toInt(),
      startMinute = args[4].toInt(),
      finHour = args[5].toInt(),
      finMinute = args[6].toInt(),
      type = (id == -1)?1:3;
    calendar.addAlarmOnce(day, mth, yr, startHour, startMinute, finHour, finMinute, type, id);
  }
}

void removeAlarm(int argc, String args[]){
  if(argc == 0){
    Serial.printf("\n Enter id of alarm to remove.");
  } else {
    calendar.removeAlarm(args[0].toInt());
  }
}

void printEvents(int argc, String args[]){
  menu.printEvents();
}

void connectPir(int argc, String args[]){
  Serial.printf("\n Connecting pir");
  if(args[0] != ""){
    int n = args[0].toInt();
    if(n < connectedNodes){
      nodes[n]->connectPir();
      Serial.printf("\n Connecting pir of node %d", n);
    } else Serial.printf("\n Node %d is not connected", n);
  }
}

void disconnectPir(int argc, String args[]){
  Serial.printf("\n Disconnecting pir");
  if(args[0] != ""){
    int n = args[0].toInt();
    if(n < connectedNodes){
      nodes[n]->disconnectPir();
      Serial.printf("\n Disconnecting pir of node %d", n);
    } else Serial.printf("\n Node %d is not connected", n);
  }
}

void connectTemp(int argc, String args[]){
  Serial.printf("\n Connecting temperature");
  if(args[0] != ""){
    int n = args[0].toInt();
    if(n < connectedNodes){
      nodes[n]->connectTemperatureSensor();
      Serial.printf("\n Connecting temperature of node %d", n);
    } else Serial.printf("\n Node %d is not connected", n);
  }
}

void disconnectTemp(int argc, String args[]){
  Serial.printf("\n Disconnecting temperature");
  if(args[0] != ""){
    int n = args[0].toInt();
    if(n < connectedNodes){
      nodes[n]->disconnectTemperatureSensor();
      Serial.printf("\n Disconnecting temperature of node %d", n);
    } else Serial.printf("\n Node %d is not connected", n);
  }
}

/*void connectCurrent(int argc, String args[]){
  Serial.printf("\n Connecting current");
  if(args[0].equalsIgnoreCase("ALL")){
    sendGlobalMessage(CONNECT_CURRENT_MSG);
    for(int i = 0; i < connectedNodes; i++){
      nodes[i]->hasCurrentSensor() = true;
    }
  } else if(args[0] != ""){
    int node = args[0].toInt();
    if(node < connectedNodes){
      sendMessage(CONNECT_CURRENT_MSG, &nodes[node]);
      nodes[node].state = PENDING_RESPONSE_STATE;
      nodes[node]->hasCurrentSensor() = true;
      Serial.printf("\n Connecting current of node %d", node);
    } else Serial.printf("\n Node %d is not connected", node);
  }
}

void disconnectCurrent(int argc, String args[]){
  Serial.printf("\n Disconnecting current");
  if(args[0].equalsIgnoreCase("ALL")){
    sendGlobalMessage(DISCONNECT_CURRENT_MSG);
    for(int i = 0; i < connectedNodes; i++){
      nodes[i]->hasCurrentSensor() = false;
    }
  } else if(args[0] != ""){
    int node = args[0].toInt();
    if(node < connectedNodes){
      sendMessage(DISCONNECT_CURRENT_MSG, &nodes[node]);
      nodes[node].state = PENDING_RESPONSE_STATE;
      nodes[node]->hasCurrentSensor() = false;
      Serial.printf("\n Disconnecting current of node %d", node);
    } else Serial.printf("\n Node %d is not connected", node);
  }
}*/

/*void connectRelay1(int argc, String args[]){
  Serial.printf("\n Connecting relay 1");
  if(args[0].equalsIgnoreCase("ALL")){
    sendGlobalMessage(CONNECT_RELAY1_MSG);
  } else if(args[0] != ""){
    int node = args[0].toInt();
    if(node < connectedNodes){
      if(isAvailable(nodes[node])){
        sendMessage(CONNECT_RELAY1_MSG, &nodes[node]);
        nodes[node].state = PENDING_RESPONSE_STATE;
        Serial.printf("\n Connecting relay 1 of node %d", node);
      } else 
        Serial.printf("\n Node %d is not available", node);
    } else Serial.printf("\n Node %d is not connected", node);
  }
}

void connectRelay2(int argc, String args[]){
  Serial.printf("\n Connecting relay 2");
  if(args[0].equalsIgnoreCase("ALL")){
    sendGlobalMessage(CONNECT_RELAY2_MSG);
  } else if(args[0] != ""){
    int node = args[0].toInt();
    if(node < connectedNodes){
      if(isAvailable(nodes[node])){
        sendMessage(CONNECT_RELAY2_MSG, &nodes[node]);
        nodes[node].state = PENDING_RESPONSE_STATE;
        Serial.printf("\n Connecting relay 2 of node %d", node);
      } else 
        Serial.printf("\n Node %d is not available", node);
    } else Serial.printf("\n Node %d is not connected", node);
  }
}

void disconnectRelay1(int argc, String args[]){
  Serial.printf("\n Disconnecting relay 1");
  if(args[0].equalsIgnoreCase("ALL")){
    sendGlobalMessage(DISCONNECT_RELAY1_MSG);
  } else if(args[0] != ""){
    int node = args[0].toInt();
    if(node < connectedNodes){
      if(isAvailable(nodes[node])){
        sendMessage(DISCONNECT_RELAY1_MSG, &nodes[node]);
        nodes[node].state = PENDING_RESPONSE_STATE;
        Serial.printf("\n Disconnecting relay 1 of node %d", node);
      } else 
        Serial.printf("\n Node %d is not available", node);
    } else Serial.printf("\n Node %d is not connected", node);
  }
}

void disconnectRelay2(int argc, String args[]){
  Serial.printf("\n Disconnecting relay 2");
  if(args[0].equalsIgnoreCase("ALL")){
    sendGlobalMessage(DISCONNECT_RELAY2_MSG);
  } else if(args[0] != ""){
    int node = args[0].toInt();
    if(node < connectedNodes){
      if(isAvailable(nodes[node])){
        sendMessage(DISCONNECT_RELAY2_MSG, &nodes[node]);
        nodes[node].state = PENDING_RESPONSE_STATE;
        Serial.printf("\n Disconnecting relay 2 of node %d", node);
      } else 
        Serial.printf("\n Node %d is not available", node);
    } else Serial.printf("\n Node %d is not connected", node);
  }
}*/

void printNodeInfo(int argc, String args[]){
  Serial.printf("\n Nodes:");
  for(int i = 0; i < connectedNodes; i++){
    Serial.printf("\n  %d - IP: %s State: ", i, nodes[i]->getIp().toString().c_str());
    switch(nodes[i]->getState()){
      case PENDING_STATE: Serial.printf("pending"); break;
      case RUNNING_STATE: Serial.printf("running"); break;
      case PENDING_RESPONSE_STATE: Serial.printf("pending response"); break;
      case ERROR_STATE: Serial.printf("error"); break;
    }
  }
}

void clearEEPROM(int argc, String args[]){
  for (int i = 0; i < MEMORY_SIZE; i++)
    EEPROM.write(i, 0);
  for(int i = 0; i < MAX_MASTER_RELAY_AMOUNT; i++){
    EEPROM.write(RELAY_MODE_DIR+i, 255);
  }
  EEPROM.commit();
  Serial.printf("\n EEPROM clear.");
}

// --- Set RTC3231 --- //
void setRTCTime(int argc, String args[]){
  if (argc != 5){
    Serial.printf("\n Enter hours, minutes, day, month, year");  
  } else {
    Serial.printf("\n Enter hours %d, minutes %d, day of month %d, month %d, year %d", 
    args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt());  

    //Prueba para settear hora. Si uso variables ya inicializadas funciona pero no vuelve a tomar la hora correcta
    uint8_t hours = args[0].toInt(); //hora
    uint8_t minutes = args[1].toInt();//atoi (args[1].substring(1, 3).c_str ());//min
    uint8_t day = args[2].toInt();//atoi (args[3].substring(1, 3).c_str ()); //dia
    uint8_t month = args[3].toInt();//atoi (args[4].substring(1, 3).c_str ());//mes 
    uint16_t year = args[4].toInt();//atoi (args[5].substring(1, 3).c_str ()); //año
    
    RtcDateTime h = RtcDateTime(year, month, day, hours, minutes, 0);
    Rtc.SetDateTime(h);
    //RtcDateTime now = Rtc.GetDateTime();
    //printDateTime(h);
  }
}

// Evento que se ejecuta si se ingresa un comando desconocido
// Recibe un parametro, el comando ingresado
void defaultEvent(int argc, String args[]){
  Serial.printf("\n Unknown command: '%s'", args[0].c_str());
}

void changeWifi(int argc, String argv[]){
  if(argc < 2){
    Serial.printf("\n Please enter new SSID (%d characters max) \n and new password (%d characters max, %d min)",
      MAX_SSID_LENGTH, MAX_PASSWD_LENGTH, MIN_PASSWD_LENGTH);
  } else {
    String aux[] = {argv[1]};
    changeSSID(1, argv);
    changePassword(1, aux);
    if(WiFi.softAP(argv[0].c_str(), argv[1].c_str())){
      Serial.printf("\n SSID: %s  Password: %s\n", argv[0].c_str(), argv[1].c_str());
    } else {
      Serial.printf("\n Failed establishing AP.\n");
    }
  }
}

void changeSSID(int argc, String argv[]){
  if((argc == 0) || (argv[0] == "") || (argv[0].length() > MAX_SSID_LENGTH))
    Serial.printf("\n Please enter new SSID (%d characters max)", MAX_SSID_LENGTH);
  else {
    writeSSID(argv[0].c_str());
    Serial.printf("\n SSID updated.");
  }
}

void changePassword(int argc, String argv[]){
  if((argc == 0) || (argv[0] == "") || (argv[0].length() > MAX_PASSWD_LENGTH) || (argv[0].length() < MIN_PASSWD_LENGTH))
    Serial.printf("\n Please enter new password (%d characters max, %d min)", MAX_PASSWD_LENGTH, MIN_PASSWD_LENGTH);
  else {
    writePassword(argv[0].c_str());
    Serial.printf("\n Password updated.");
  }
}

void checkConnection(){
  if(WiFi.status() == WL_CONNECTED){
    thing.handle();
  } else if((millis() - reconnectionTime) > 5000 ) {
    Serial.printf("\n Reconnectando a wifi...");
    WiFi.reconnect();
    reconnectionTime = millis();
  }
}

void initWifi(){

  // Para solucionar bug de reconexión
  if (WiFi.SSID() != "") {
    Serial.printf("\nWiFi credentials set in flash, wiping them");
    WiFi.disconnect();
  }
  if (WiFi.getAutoConnect()) {
    Serial.printf("\nDisabling auto-connect");
    WiFi.setAutoConnect(false);
  }
  WiFi.persistent(false);
  // -----------------
  
  const char* ssid = readSSID();
  const char* passwd = readPassword();
  WiFi.mode(WIFI_AP);
  if(WiFi.softAP(ssid, passwd, 1, 1)){ // Canal 1 (por defecto), y SSID oculta
    Serial.printf("\n SSID: %s  Password: %s\n", ssid, passwd);
  } else {
    Serial.printf("\n Failed establishing AP.\n");
  }
  char externalSSID[MAX_SSID_LENGTH] = "my_ap",
    externalPasswd[MAX_PASSWD_LENGTH] = "my_password",
    externalIp[16] = "192.168.100.1";

  if(1){//readExternalWifiInfo(externalSSID, externalPasswd, externalIp)){
    Serial.printf("\n Connecting to wifi %s.\n", externalSSID);
    int tries = 0;
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(externalSSID, externalPasswd);
    while((WiFi.status() != WL_CONNECTED) && (tries < 60)){
      tries++;
      delay(500);
      Serial.printf(".");
    }
    if(tries < 60){
      Serial.printf("\n Connected to wifi.");
      thing.handle();                  // Fuerza la conexión a thinger, para poder enviar el estado del dht luego
      panel.connect(externalIp, DEFAULT_EXT_PORT);
    } else Serial.printf("\n Couldn't connect to wifi.");
  } else Serial.printf("\n No external wifi settings found.");
}

const char* readSSID(){
  char* ssid = (char*)malloc(MAX_SSID_LENGTH*sizeof(char));
  for(int i = 0; i < MAX_SSID_LENGTH; i++){
    ssid[i] = (char)EEPROM.read(i+SSID_DIR);
    if(ssid[i] == '\0') break;
  }
  return (ssid[0] != '\0')? ssid : DEFAULT_SSID;
}

const char* readPassword(){
  char* passwd = (char*)malloc(MAX_PASSWD_LENGTH*sizeof(char));
  for(int i = 0; i < MAX_PASSWD_LENGTH; i++){
    passwd[i] = (char)EEPROM.read(i+PASSWD_DIR);
    if(passwd[i] == '\0') break;
  }
  return (passwd[0] != '\0')? passwd : DEFAULT_PASSWD;
}

bool readExternalWifiInfo(char* ssid, char* passwd, char* ip){
  char aux[MAX_SSID_LENGTH];
  bool read = false;
  for(int i = 0; i < MAX_SSID_LENGTH; i++){
    aux[i] = (char)EEPROM.read(i+EXT_SSID_DIR);
    if(aux[i] == '\0') break;
    else read = true;
  }
  strncpy(ssid, aux, MAX_SSID_LENGTH);
  if(read) read = false;
  else return read;
  for(int i = 0; i < MAX_PASSWD_LENGTH; i++){
    aux[i] = (char)EEPROM.read(i+EXT_PASSWD_DIR);
    if(aux[i] == '\0') break;
    else read = true;
  }
  strncpy(passwd, aux, MAX_PASSWD_LENGTH);
  if(read) read = false;
  else return read;
  int addr[EXT_IP_LENGTH];
  for(int i = 0; i < EXT_IP_LENGTH; i++)
    addr[i] = EEPROM.read(i+EXT_IP_DIR);
  read = addr[0] != 0;
  snprintf(ip, 16, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
  return read;
}

void changeExternalWifiInfo(String ssid, String passwd, String ip){
  for(int i = 0; i < ssid.length(); i++)
    EEPROM.write(i+EXT_SSID_DIR, ssid[i]);
  if(ssid.length() < MAX_SSID_LENGTH)
    EEPROM.write(ssid.length()+EXT_SSID_DIR, '\0');
  for(int i = 0; i < passwd.length(); i++)
    EEPROM.write(i+EXT_PASSWD_DIR, passwd[i]);
  if(passwd.length() < MAX_PASSWD_LENGTH)
    EEPROM.write(passwd.length()+EXT_PASSWD_DIR, '\0');
  IPAddress aux = IPAddress();
  aux.fromString(ip);
  for(int i = 0; i < 4; i++)
    EEPROM.write(EXT_IP_DIR+i, aux[i]);
  EEPROM.commit();
  Serial.printf("\n Saving wifi info:\n  SSID: %s, pass: %s, ip: %s", ssid.c_str(), passwd.c_str(), ip.c_str());
}

void writeSSID(const char* ssid){
  for(int i = 0; i < MAX_SSID_LENGTH; i++){
    EEPROM.write(i+SSID_DIR, ssid[i]);
  }
  EEPROM.commit();
}

void writePassword(const char* passwd){
  for(int i = 0; i < MAX_PASSWD_LENGTH; i++){
    EEPROM.write(i+PASSWD_DIR, passwd[i]);
  }
  EEPROM.commit();
}

int createWifiPacket(int packet[]){
  const char* ssid = readSSID();
  const char* passwd = readPassword();
  char c;
  int pos = 0;
  Serial.printf("\n SSID ");
  for(int i = 0; i < MAX_SSID_LENGTH; i++){
    packet[pos++] = ssid[i];
    Serial.printf(" %c", packet[pos-1]);
    if(!ssid[i]) break;
  }
  Serial.printf("\n Pass ");
  for(int i = 0; i < MAX_PASSWD_LENGTH; i++){
    packet[pos++] = passwd[i];
    Serial.printf(" %c", packet[pos-1]);
    if(!passwd[i]) break;
  }
  return pos;
}

void sendWifiSettings(){
  int packet[MAX_PACKET_SIZE];
  int length = createWifiPacket(packet);
  sendGlobalPacket(WIFI_SETTINGS_MSG, packet, length);
}

void initRelayGroups(){
  for(int i = 0; i < GROUP_AMOUNT; i++){
    char c = (char)EEPROM.read(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i));
    if(c != '\0'){
      relayGroups[i].isConnected = true;
      relayGroups[i].name += c;
      for(int j = 1; j < GROUP_NAME_LENGTH; j++){
        if(c = (char)EEPROM.read(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+j))
          relayGroups[i].name += c;
        else break;
      }
      relayGroups[i].ignorePir = EEPROM.read(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+GROUP_NAME_LENGTH) == 1;
      Serial.printf("\n Grupo %s, pir: %d", relayGroups[i].name.c_str(), relayGroups[i].ignorePir);
      if(!relayGroups[i].ignorePir){
        byte aux[2];
        aux[0] = EEPROM.read(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+GROUP_NAME_LENGTH+1);
        aux[1] = EEPROM.read(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+GROUP_NAME_LENGTH+2);
        memcpy(&relayGroups[i].motionDetectionTime, &aux, 2);
        Serial.printf("\n time: %d", relayGroups[i].motionDetectionTime);
      }
    }
  }
}

void storeRelayGroupSettings(){
  for(int i= 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      for(int j = 0; j < GROUP_NAME_LENGTH; j++)
        EEPROM.write(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+j, relayGroups[i].name[j]);
      EEPROM.write(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+GROUP_NAME_LENGTH, (relayGroups[i].ignorePir)?1:0);
      byte aux[2];
      memcpy(&aux, &relayGroups[i].motionDetectionTime, 2);
      EEPROM.write(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+GROUP_NAME_LENGTH+1, aux[0]);
      EEPROM.write(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+GROUP_NAME_LENGTH+2, aux[1]);
    } else
      for(int j= 0; j < GROUP_INFO_LENGTH; j++)
        EEPROM.write(GROUP_INFO_DIR+(GROUP_INFO_LENGTH*i)+j, 0);
  }
  EEPROM.commit();
}

int groupNumber(String name){
  for(int i = 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].name == name){
      return i;
    }
  }
  return -1;
}

void readNodeInfo(){
  for(int i = 0; i < NAME_LENGTH; i++)
    nodeName[i] = EEPROM.read(NAME_DIR+i);
  if(!nodeName[0]) strncpy(nodeName, "unnamed", NAME_LENGTH);
  Serial.printf("\n Name: %s", nodeName);
}

void storeNodeInfo(const char* name){
  strncpy(nodeName, name, NAME_LENGTH);
  for(int i = 0; i < NAME_LENGTH; i++){
    EEPROM.write(NAME_DIR+i, name[i]);
    if(name[i] == '\0') break;
  }
  EEPROM.commit();
  Serial.printf("\n Name: %s", nodeName);
}

void checkIRCode(){
  if(recordIRCode){
    Serial.printf("\n Recording code...");
    if(irrecord.record())
      Serial.printf("\n Code recorded.");
    else
      Serial.printf("\n Code recording failed.");
    recordIRCode = false;
  }
}

// Panel de control
void initControlPanel(){
  readNodeInfo();
  panel = ControlPanel();
  panel.createHandler("name", panelName);
  panel.createHandler("groups", panelGroups);
  panel.createHandler("turnon_group", panelTurnOnGroup);
  panel.createHandler("turnoff_group", panelTurnOffGroup);
  panel.createHandler("create_group", panelCreateGroup);
  panel.createHandler("delete_group", panelDeleteGroup);
  panel.createHandler("modify_group", panelModifyGroup);
  panel.createHandler("weekly_alarm", panelAddWeeklyAlarm);
  panel.createHandler("alarm_once", panelAddAlarmOnce);
  panel.createHandler("delete_alarm", panelDeleteAlarm);
  panel.createHandler("extwifi", panelExternalWifi);
  panel.createHandler("connect", panelConnect);
  panel.createHandler("disconnect", panelDisconnect);
  panel.createHandler("pir_settings", panelPirSettings);
  panel.createHandler("temperature", panelGetTemperature);
  panel.createHandler("slaves", panelGetSlaveInfo);
  panel.createHandler("relay_info", panelGetRelayInfo);
  panel.createHandler("time", panelTime);
  panel.createHandler("intwifi", panelChangeWifi);
  panel.createHandler("reset", panelReset);
  panel.createHandler("record_ir", panelRecordIR);
  panel.createHandler("ir_transmit", panelTransmitIrCode);
  panel.createHandler("ir_transmit_server", panelTransmitIrCodeToServer);
  panel.createHandler("pairing_mode", panelStartPairing);
  panel.createHandler("clear_eeprom", panelClearEEPROM);
}

void panelClearEEPROM(int args, String argv[]){
  String* arg = NULL;
  clearEEPROM(0,argv);
  panel.addToResponse("ok");
}

void panelName(int args, String argv[]){
  switch(args){
    case 0:
      Serial.printf("\n Sending name to panel");
      panel.addToResponse(nodeName);
      break;
    case 1:
      Serial.printf("\n New name: %s", argv[0].c_str());
      storeNodeInfo(argv[0].c_str());
      panel.addToResponse("ok");
      break;
  }
}

void panelGroups(int args, String argv[]){
  bool noGroups = true;
  for(int i = 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      char response[20];
      snprintf(response, 20, "%s %s", relayGroups[i].name.c_str(), (relayGroups[i].isOn)?"on":"off");
      panel.addToResponse(response);
      noGroups = false;
    }
  }
  if(noGroups) panel.addToResponse("no_groups");
}

void panelTurnOnGroup(int args, String argv[]){
  if(argv[0] == "all"){
    turnOnEverythingEvent(0);
  } else {
    int g;
    if((g = groupNumber(argv[0])) != -1){
      Serial.printf("\n Group: %d", g);
      if(relayGroups[g].isConnected){
        turnOnGroup(g);
        if(!relayGroups[g].ignorePir){
          turnOnPir();
          Serial.printf("\n motion: %d", motionAlarm);
          if(motionAlarm == ALARM_NOT_SET){
            sensors.pir.nextSleepingGroup = g;
            motionAlarm = Alarm.timerOnce(relayGroups[g].motionDetectionTime, sleepGroups);
            currentMotionDetectionTime = relayGroups[g].motionDetectionTime;
          }
        }
      }
    }
  }
  panel.addToResponse("ok");
}

void panelTurnOffGroup(int args, String argv[]){
  if(argv[0] == "all"){
    turnOffEverythingEvent(0);
  } else {
    int g;
    if((g = groupNumber(argv[0])) != -1){
      if(relayGroups[g].isConnected){
        Serial.printf("\n Turn off %s", relayGroups[g].name.c_str());
        turnOffGroup(g, false);
        if(!relayGroups[g].ignorePir)
          turnOffPir();
      }
    }
  }
  panel.addToResponse("ok");
}

void panelCreateGroup(int argc, String argv[]){
  int i;
  for(i = 0; i < GROUP_AMOUNT; i++)
    if(!relayGroups[i].isConnected) break;
  if(!relayGroups[i].isConnected){
    relayGroups[i].isConnected = true;
    relayGroups[i].name = argv[0];
    relayGroups[i].ignorePir = (argv[1] == "ignore_pir");
    if(!relayGroups[i].ignorePir)
      relayGroups[i].motionDetectionTime = argv[1].toInt();
    else
      relayGroups[i].motionDetectionTime = DEFAULT_MOTION_DET_TIME;
    storeRelayGroupSettings();
  }
  panel.addToResponse("ok");
}

void panelDeleteGroup(int argc, String argv[]){
  int g;
  if((g = groupNumber(argv[0])) != -1){ // Si existe el grupo devuelve el indice
    relayGroups[g].isConnected = false;
    for(int i = 0; i < MAX_MASTER_RELAY_AMOUNT; i++){
      if(relays[i].connected && (relays[i].group == g)){
        relays[i].connected = false;
      }
    }
    for(int i = 0; i < connectedNodes; i++)
      nodes[i]->disconnectFromGroup(g);
    calendar.removeAlarmsWithEventAndId(3, g); // Elimina las alarmas para el grupo correspondiente
    storeRelayGroupSettings();
    storeSensorInfo();
  }
  panel.addToResponse("ok");
}

void panelModifyGroup(int argc, String argv[]){
  int g;
  if((g = groupNumber(argv[0])) != -1){ // Si existe el grupo devuelve el indice
    relayGroups[g].name = argv[1];
    relayGroups[g].ignorePir = (argv[2] == "ignore_pir");
    if(!relayGroups[g].ignorePir)
      relayGroups[g].motionDetectionTime = argv[2].toInt();
    else if(relayGroups[g].isOn && relayGroups[g].isSleeping){
      Serial.printf("\n Despertando al grupo %d", g);
      turnOnGroup(g);
    }
    storeRelayGroupSettings();
  }
  panel.addToResponse("ok");
}

void panelConnect(int argc, String argv[]){
  Node* node;
  if(argv[0] == "master"){
    if(argv[1] == "pir"){
      disconnectMasterDht();
      connectMasterPir();
      Serial.printf("\n PIR CONNECTED");
    } else if(argv[1] == "temp"){
      disconnectMasterPir();
      connectMasterDht();
      Serial.printf("\n DHT CONNECTED");
    } else if (argv[1] == "relay"){
      int r = argv[2].toInt(); // Determina que relay se conecta
      relays[r].connected = true;
      relays[r].group = groupNumber(argv[3]);
      relays[r].on = false;
      storeSensorInfo();
      Serial.printf("\n RELAY %s CONNECTED", argv[2].c_str());
    }
  } else if(node = getNode(argv[0].toInt())){
    if(argv[1] == "pir"){
      node->connectPir();
      Serial.printf("\n PIR CONNECTED SLAVE %s", argv[0].c_str());
    } else if(argv[1] == "temp"){
      node->connectTemperatureSensor();
      Serial.printf("\n DHT CONNECTED SLAVE %s", argv[0].c_str());
    } else if(argv[1] == "relay"){
      node->connectRelay(argv[2].toInt(), groupNumber(argv[3]));
      Serial.printf("\n RELAY CONNECTED SLAVE %s TO GROUP %d", argv[0].c_str(), groupNumber(argv[3]));
    } else if(argv[1] == "ircontrol"){
      node->connectIrControl(groupNumber(argv[2]));
      Serial.printf("\n IR CONNECTED SLAVE %d", argv[0].toInt());
    }
  } else {
    Serial.printf("\n Node %d not found", argv[0].toInt());
  }
  panel.addToResponse("ok");
}

void panelDisconnect(int argc, String argv[]){
  Node* node;
  if(argv[0] == "master"){
    if(argv[1] == "pir"){
      disconnectMasterPir();
      Serial.printf("\n PIR DISCONNECTED");
    } else if(argv[1] == "temp"){
      disconnectMasterDht();
      Serial.printf("\n DHT DISCONNECTED");
    } else if (argv[1] == "relay"){
      int r = argv[2].toInt(); // Determina que relay se conecta
      relays[r].connected = false;
      digitalWrite(relays[r].pin, LOW);
      storeSensorInfo();
      Serial.printf("\n RELAY %s DISCONNECTED", argv[2].c_str());
    }
  } else if(node = getNode(argv[0].toInt())){
    if(argv[1] == "pir"){
      node->disconnectPir();
      Serial.printf("\n PIR DISCONNECTED SLAVE %s", argv[0].c_str());
    } else if(argv[1] == "temp"){
      node->disconnectTemperatureSensor();
      Serial.printf("\n DHT DISCONNECTED SLAVE %s", argv[0].c_str());
    } else if (argv[1] == "relay"){
      node->disconnectRelay(argv[2].toInt());
      Serial.printf("\n RELAY DISCONNECTED SLAVE %s", argv[0].c_str());
    } else if(argv[1] == "ircontrol"){
      node->disconnectIrControl();
      Serial.printf("\n IR DISCONNECTED SLAVE %d", argv[0].toInt());
    }
  }
  panel.addToResponse("ok");
}

void panelPirSettings(int argc, String argv[]){
  if(argc == 0){
    panel.addToResponse(String(sensors.pir.inactivityTime).c_str());
  } else {
    sensors.pir.inactivityTime = argv[0].toInt();
    storeSensorInfo();
    panel.addToResponse("ok");
  }
}

void panelGetTemperature(int argc, String argv[]){
  bool read = false;
  if(sensors.dht.isConnected){
    int temp;
    short int hum;
    if(ambientRead){
      char* text = (char*)malloc(sizeof(char)*40);
      snprintf(text, 40, "%d&%d", temperature, humidity);
      panel.addToResponse(text);
      read = true;
    }
  }
  for(int i = 0; i < connectedNodes; i++){
    if(nodes[i]->hasTemperatureSensor()){
      nodes[i]->sendMessage(SENSOR_DATA_MSG);
      sensors.dht.requestedFromPanel = true;
      break;
    }
  }
  if(!read) panel.addToResponse("no_sensor");
}

void panelAddWeeklyAlarm(int argc, String args[]){
  int day = args[0].toInt(),
    startHour = args[1].toInt(),
    startMinute = args[2].toInt(),
    finHour = args[3].toInt(),
    finMinute = args[4].toInt(),
    type = args[5].toInt();
  int alarmId = -1;
  if(type == 1) // Tipo 1 controla todos los grupos
    alarmId = calendar.addWeeklyAlarm(day, startHour, startMinute, finHour, finMinute, type, 0);
  else {        // Tipo 3 especifica el nombre del grupo
    int id = groupNumber(args[6]);
    alarmId = calendar.addWeeklyAlarm(day, startHour, startMinute, finHour, finMinute, type, id);
  }
  char idResponse[3];
  snprintf(idResponse, 3, "%d", alarmId);
  panel.addToResponse(idResponse);
  Serial.printf("\n Alarma %d creada.", alarmId);
}

void panelAddAlarmOnce(int argc, String args[]){
  int day = args[0].toInt(),
    mth = args[1].toInt(),
    yr = args[2].toInt(),
    startHour = args[3].toInt(),
    startMinute = args[4].toInt(),
    finHour = args[5].toInt(),
    finMinute = args[6].toInt(),
    type = args[7].toInt();
  int alarmId = -1;
  if(type == 1) // Tipo 1 controla todos los grupos
    alarmId = calendar.addAlarmOnce(day, mth, yr, startHour, startMinute, finHour, finMinute, type, 0);
  else{         // Tipo 3 especifica el nombre del grupo
    int id = groupNumber(args[8]);
    alarmId = calendar.addAlarmOnce(day, mth, yr, startHour, startMinute, finHour, finMinute, type, id);
  }
  char idResponse[3];
  snprintf(idResponse, 3, "%d", alarmId);
  panel.addToResponse(idResponse);
  Serial.printf("\n Alarma %d creada.", alarmId);
}

void panelDeleteAlarm(int argc, String args[]){
  int id = args[0].toInt();
  calendar.removeAlarm(id);
  panel.addToResponse("ok");
}

void panelExternalWifi(int argc, String args[]){
  changeExternalWifiInfo(args[0], args[1], args[2]);
  panel.addToResponse("ok");
}

void panelGetSlaveInfo(int argc, String argv[]){
  for(int i = 0; i < connectedNodes; i++){
    char message[20];
    snprintf(message, 20, "%d", nodes[i]->getIp()[3]);
    panel.addToResponse(message);
  }
  if(!connectedNodes){
    panel.addToResponse("no_slaves");
  }
}

void panelGetRelayInfo(int argc, String argv[]){
  char message[5];
  snprintf(message, 5, "%d", MAX_MASTER_RELAY_AMOUNT);
  panel.addToResponse(message);
  snprintf(message, 5, "%d", MAX_SLAVE_RELAY_AMOUNT);
  panel.addToResponse(message);
}

void panelTime(int argc, String argv[]){
  if(!argc){
    time_t t = now();
    panel.addToResponse(hour(t));
    panel.addToResponse(minute(t));
    panel.addToResponse(weekday(t));
    panel.addToResponse(day(t));
    panel.addToResponse(month(t));
    panel.addToResponse(year(t));
  } else {
    setRTCTime(argc, argv);
    setTime(argv[0].toInt(), argv[1].toInt(), 0, argv[2].toInt(), argv[3].toInt(), argv[4].toInt());
    panel.addToResponse("ok");
    resetAlarms(); // Las alarmas dejan de funcionar cuando se setea el tiempo
  }
}

void panelChangeWifi(int argc, String argv[]){
  if(argc == 0){
    panel.addToResponse(readSSID());
    panel.addToResponse(readPassword());
  } else {
    if((argv[0].length() < MAX_SSID_LENGTH) && (argv[1].length() >  MIN_PASSWD_LENGTH) && (argv[1].length() < MAX_PASSWD_LENGTH)){
      changeWifi(argc, argv);
    }
    panel.addToResponse("ok");
  }
}

void panelReset(int argc, String argv[]){
  resetFromPanel = true;
  panel.addToResponse("ok");
}

void panelRecordIR(int argc, String argv[]){
  recordIRCode = true;
  panel.addToResponse("ok");
}

void panelTransmitIrCodeToServer(int argc, String argv[]){
  if(irrecord.codeRecorded()){
    unsigned short code[IR_CODE_LENGTH];
    int len = irrecord.getValues(code);
    for(int i = 0; i < len; i++){
      char value[5];
      snprintf(value, 5, "%d", code[i]);
      panel.addToResponse(value);
    }
  } else {
    panel.addToResponse("no_code");
  }
}

void panelTransmitIrCode(int argc, String argv[]){
  if(irCodeComplete){
    Serial.printf("\n Sobreescribiendo codigo ir.");
    irCodeLength = 0;
    irCodeComplete = false;
  }
  for(int i = 0; i < argc; i++){
    int code = argv[i].toInt();
    if(code == -1){
      Serial.printf("\n Longitud de codigo ir: %d\n", irCodeLength);
      /*for(int j = 0; j < irCodeLength; j++){
        if(j % 15 == 0)
          Serial.printf("\n ");
        Serial.printf("%04d ", irCode[j]);
      }*/
      irCodeComplete = true;
      break;
    }
    irCode[irCodeLength] = code;
    irCodeLength++;
  }
  if(irCodeComplete){
    // Transmisión a esclavos
    int packet[MAX_PACKET_SIZE-2];
    int len = 0, transmitted = 0;
    byte v[2];
    memcpy(&v, &irCodeLength, 2);
    packet[len++] = v[0];
    packet[len++] = v[1];
    sendGlobalPacket(IR_LENGTH_MSG, packet, len);
    while(transmitted < irCodeLength){
      len = 0;
      while((len < (MAX_PACKET_SIZE-2)) && (transmitted < irCodeLength)){
        byte v[2];
        memcpy(&v, &irCode[transmitted++], 2);
        packet[len++] = v[0];
        packet[len++] = v[1];
      }
      sendGlobalPacket(IR_CODE_MSG, packet, len);
    }
    panel.addToResponse("ok");
  }
}

void startPairing(int seconds = 60){
  Serial.printf("\n Starting pairing mode");
  startPairingMode = false;
  Alarm.free(stateCheckAlarm);
  stateCheckAlarm = ALARM_NOT_SET;
  int wifiPacket[MAX_PACKET_SIZE];
  int wifiLength = createWifiPacket(wifiPacket);
  connectedNodes = 0;
  if(WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWD)){
    int start = millis();
    while((millis() - start) < seconds*1000){
      while(udp.parsePacket()){
        Node* node;
        byte packet[MAX_PACKET_SIZE];
        int length = udp.read(packet, MAX_PACKET_SIZE);
        if ( (node = getNode(udp.remoteIP())) ) {                              // Si la longitud es mayor a 2 bytes se trata de un paquete
          int message = (int)packet[0];
          if( message == IP_MSG ){
            sendNewIP(node);
          } else if(message == WIFI_SETTINGS_MSG){
            Serial.printf("\n Sending wifi settings");
            node->sendPacket(WIFI_SETTINGS_MSG, wifiPacket, wifiLength);
          } else if(message == CONNECTION_MSG){
            Serial.printf("\n Node connecting again");
            node->reconnect();
          } else if(message == RECEIVED_MSG){
            Serial.printf("\n Ack received from node %d", node->getIp()[3]);
            node->receiveAck();
          } else node->sendAck();
        } else if((int)packet[0] == CONNECTION_MSG){
          printActualTime();                                                // Si es un nodo nuevo
          Serial.printf(" New node: %s", udp.remoteIP().toString().c_str());
          if (connectedNodes < MAX_NODES) {                                 // Se crea una instancia para el nodo
            nodes[connectedNodes] = new Node(udp, udp.remoteIP());
            nodes[connectedNodes]->sendAck();
            connectedNodes++;
            visualMenu.update();
          } else
            Serial.printf("Maximun number of nodes reached.");
        }
      }
    }
  } else {
    Serial.printf("\n Error while starting pairing.");
  }
  Serial.printf("\n Exiting pairing mode");
  const char* ssid = readSSID();
  const char* passwd = readPassword();
  WiFi.softAP(ssid, passwd);
  //connectedNodes = 0;
  for(int i = 0; i < connectedNodes; i++) nodes[i]->clearMessages();
  stateCheckAlarm = Alarm.timerRepeat(STATE_CHECK_TIME, checkNodeStatus);
}

void panelStartPairing(int argc, String argv[]){
  startPairingMode = true;
  if(argc == 1)
    pairingTime = argv[0].toInt();
  panel.addToResponse("ok");
}

// Menu 
LCDSubmenu* mainMenu;
LCDSubmenu* menuEstado;
LCDSubmenu* menuInteraccion;
LCDSubmenu* listaAlarmas;
LCDSubmenu* menuControlar;
LCDSubmenu* controlarMaestro;
LCDSubmenu* menuAgregar;
LCDSubmenu* alarmaGlobal;
LCDSubmenu* alarmaRelays;
LCDSubmenu* menuGrupos;
LCDSubmenu* interaccionGrupos;
LCDSubmenu* ajustarPir;
LCDSubmenu* nodeMenu[MAX_NODES];
LCDForm* alarmaOcasional;
LCDForm* alarmaSemanal;
LCDForm* ajustarWifi;
LCDForm* ajustarHora;
LCDForm* crearGrupo;
LCDForm* weeklyAlarmForms[GROUP_AMOUNT+1];
LCDForm* alarmOnceForms[GROUP_AMOUNT+1];

void initVisualMenu(){
  // Menu principal
  mainMenu = new LCDSubmenu("POWER-AR", initVisualTime);
  mainMenu->setUpdateFunction(updateVisualTime);
  menuInteraccion = new LCDSubmenu("Interaccion");
  menuEstado = new LCDSubmenu("Estado de nodos", "No hay nodos.", visualInitNodes);
  LCDSubmenu* menuAlarmas = new LCDSubmenu("Alarmas");
  LCDSubmenu* menuAjustes = new LCDSubmenu("Ajustes");
  mainMenu->addItem(menuInteraccion);
  mainMenu->addItem(menuAlarmas);
  mainMenu->addItem(menuAjustes);
  mainMenu->addItem(menuEstado);
  menuEstado->setUpdateFunction(visualUpdateNodes);
  // Menu de interaccion
  interaccionGrupos = new LCDSubmenu("Dispositivos", updateGroupControls);
  menuInteraccion->addItem(interaccionGrupos);
  LCDFunction* sensorDataFunction = new LCDFunction("Ambiente", getVisualSensorData);
  sensorDataFunction->setUpdateFunction(updateSensorData);
  lcd.createChar(0, customChar);
  menuInteraccion->addItem(sensorDataFunction);
  menuControlar = new LCDSubmenu("Controlar", "No hay nodos.", updateControlNodes);
  menuInteraccion->addItem(menuControlar);
  // Menu de alarmas
  listaAlarmas = new LCDSubmenu("Ver alarmas", "No hay alarmas.", showAlarms);
  menuAlarmas->addItem(listaAlarmas);
  //menuAgregar = new LCDSubmenu("Nueva alarma", updateNewAlarm);
  //alarmaRelays = new LCDSubmenu("Alarma por relay", updateRelayAlarms);
  /*alarmaOcasional = new LCDForm("Ocasional", newAlarmOnce, updateAlarmForm);
  alarmaOcasional->addField("Dia", TYPE_INT, 1, 1, 31);
  alarmaOcasional->addField("Mes", TYPE_INT, 1, 1, 12);
  alarmaOcasional->addField("Anio", TYPE_INT, 2018);
  alarmaOcasional->addField("Hora", TYPE_INT, 12, 0, 23);
  alarmaOcasional->addField("Minutos", TYPE_INT, 0, 0, 59);
  alarmaOcasional->addField("Hora de fin", TYPE_INT, 12, 0, 23);
  alarmaOcasional->addField("Minutos de fin", TYPE_INT, 0, 0, 59);
  alarmaOcasional->makeStatic();
  alarmaSemanal = new LCDForm("Semanal", newAlarmWeekly, updateWeeklyAlarmForm);
  alarmaSemanal->addField("Dia de semana", TYPE_INT);
  alarmaSemanal->addField("Hora", TYPE_INT);
  alarmaSemanal->addField("Minutos", TYPE_INT);
  alarmaSemanal->addField("Hora de fin", TYPE_INT);
  alarmaSemanal->addField("Minutos de fin", TYPE_INT);
  alarmaSemanal->makeStatic();
  alarmaGlobal  = new LCDSubmenu("Global");
  alarmOnceForms[0] = alarmaOcasional->copy(-1);
  alarmaGlobal->addItem(alarmOnceForms[0]);
  weeklyAlarmForms[0] = alarmaSemanal->copy(-1);
  alarmaGlobal->addItem(weeklyAlarmForms[0]);
  alarmaGlobal->makeStatic();
  //menuAgregar->addItem(alarmaRelays);
  menuAlarmas->addItem(menuAgregar);*/
  // Menu de ajustes
  ajustarHora = new LCDForm("Cambiar hora", changeTime, updateTimeForm);
  ajustarHora->addField("Hora", TYPE_INT, 12, 0, 23);
  ajustarHora->addField("Minutos", TYPE_INT, 0, 0, 59);
  ajustarHora->addField("Dia", TYPE_INT, 1, 1, 7);
  ajustarHora->addField("Mes", TYPE_INT, 1, 1, 12);
  ajustarHora->addField("Anio", TYPE_INT, 2018, 2018, 2040);
  LCDForm* ajustarWifiExterno = new LCDForm("Wifi Servidor", visualChangeExternalWifi);
  ajustarWifiExterno->addField("SSID", TYPE_STRING);
  ajustarWifiExterno->addField("Contr.", TYPE_STRING);
  ajustarWifiExterno->addField("IP", TYPE_STRING);
  LCDFunction* limpiarEEPROM = new LCDFunction("Limpiar EEPROM", visualClearEEPROM, "Limpiando...");
  /*ajustarWifi = new LCDForm("Cambiar WiFi", changeWifiSettings, updateWifiForm);
  ajustarWifi->addField("Nombre", TYPE_STRING);
  ajustarWifi->addField("Contr.", TYPE_STRING);*/
  //LCDSubmenu* ajustarNodo = new LCDSubmenu("Nodos");
  //ajustarPir = new LCDSubmenu("Pir", updatePirConfig);
  menuAjustes->addItem(ajustarHora);
  menuAjustes->addItem(ajustarWifiExterno);
  menuAjustes->addItem(limpiarEEPROM);
  //menuAjustes->addItem(ajustarWifi);
  //menuAjustes->addItem(ajustarNodo);
  menuGrupos = new LCDSubmenu("Dispositivos", updateRelayGroups);
  menuAjustes->addItem(menuGrupos);
  menuAjustes->addItem(new LCDFunction("Reiniciar", visualReset, "Reiniciando..."));
  /*crearGrupo = new LCDForm("Nuevo grupo", newGroupForm);
  crearGrupo->makeStatic();
  crearGrupo->addField("Nombre", TYPE_STRING);*/
  // Menu acerca de
  char* body = "Power-Ar v1.0";
  mainMenu->addItem(new LCDText("Acerca de", body));
  // Inicializa menu principal
  visualMenu.addHome(mainMenu);
}

char* visualClearEEPROM(int id){
  String* argv = NULL;
  clearEEPROM(0,argv);
  return "EEPROM limpia.";
}

char* visualChangeExternalWifi(int argc, String argv[], int id){
  changeExternalWifiInfo(argv[0], argv[1], argv[2]);
  return "Wifi del servidor cambiado.";
}

void turnOnGroup(int group){
  for(int i = 0; i < MAX_MASTER_RELAY_AMOUNT; i++){
    if((relays[i].connected) && (relays[i].group == group)){
      //Serial.printf("\n Prendiendo relay %d en pin %d", i, relays[i].pin);
      digitalWrite(relays[i].pin, HIGH);
      Serial.printf("\n Turning on relay %d", i);
      relays[i].on = true;
    }
  }
  int packet[]= {group};
  sendGlobalPacket(TURN_ON_GROUP_MSG, packet, 1);
  relayGroups[group].isOn = true;
  relayGroups[group].isSleeping = false;
  thing.stream(thing["groups_state"]);
  thing.stream(thing["actuator_info"]);
  thing.write_bucket("ActuatorInfo", "actuator_info");
  char bucket[50];
  snprintf(bucket, 50, "%sGroups", nodeName);
  thing.write_bucket(bucket, "groups_state");
  thing.call_endpoint("NodeRedGroupState", thing["complete_groups_state"]);
}

void turnOffGroup(int group, bool sleep){
  for(int i = 0; i < MAX_MASTER_RELAY_AMOUNT; i++){
    if((relays[i].connected) && (relays[i].group == group)){
      //Serial.printf("\n Apagando relay %d en pin %d", i, relays[i].pin);
      digitalWrite(relays[i].pin, LOW);
      relays[i].on = false;
      Serial.printf("\n Turning off relay %d", i);
    }
  }
  int packet[]= {group};
  sendGlobalPacket(TURN_OFF_GROUP_MSG, packet, 1);
  if(sleep){
    relayGroups[group].isOn = true;
    relayGroups[group].isSleeping = true;
  } else {
    relayGroups[group].isOn = false;
  }
  thing.stream(thing["groups_state"]);
  thing.stream(thing["actuator_info"]);
  thing.write_bucket("ActuatorInfo", "actuator_info");
  char bucket[50];
  snprintf(bucket, 50, "%sGroups", nodeName);
  thing.write_bucket(bucket, "groups_state");
  thing.call_endpoint("NodeRedGroupState", thing["complete_groups_state"]);
}

void sleepGroup(int group){
  turnOffGroup(group, true);
}

// Devuelve que cantidad de actuadores estan prendidos
// NO FUNCIONA PARA CONTROLES IR
void activeActuators(int* relayAmount, int* irAmount){
  int relay = 0;
  int ir = 0;
  for(int i = 0; i < MAX_MASTER_RELAY_AMOUNT; i++)
    if(relays[i].on) relay++;

  for(int i = 0; i < GROUP_AMOUNT; i++) {
    if(relayGroups[i].isConnected && relayGroups[i].isOn && !relayGroups[i].isSleeping){
      for(int j = 0; j < connectedNodes; j++){
        for(int k = 0; k < MAX_SLAVE_RELAY_AMOUNT; k++){
          if(nodes[j]->isRelayInGroup(k, i)){
            relay++;
          }
        }
      }
    }
  }
  
  *relayAmount = relay;
  *irAmount = ir;
}

char* manualTurnOnGroup(int group){
  turnOnGroup(group);
  turnOnPir();
  return "Grupo prendido.";
}

char* manualTurnOffGroup(int group){
  turnOffGroup(group, false);
  turnOffPir();
  return "Grupo apagado.";
}

void updatePirConfig(int id){
  ajustarPir->empty();
  LCDForm* in = new LCDForm("Inactividad", setInactivity);
  in->addField("Minutos", TYPE_INT, floor(sensors.pir.inactivityTime/60), 0, 60);
  in->addField("Segundos", TYPE_INT, sensors.pir.inactivityTime - floor(sensors.pir.inactivityTime/60), 0, 59);
  in->makeDisposable();
}

char* setInactivity(int argc, String argv[], int id){
  sensors.pir.inactivityTime = argv[0].toInt()*60 + argv[1].toInt();
  storeSensorInfo();
  return "Tiempo configurado.";
}

void initVisualTime(int id){
  updateVisualTime(id);
}

char* updateVisualTime(int id){
  time_t t = now();
  char title[MAX_LINE_WIDTH];
  snprintf(title, MAX_LINE_WIDTH, "PowerAR %02d:%02d %02d/%02d", hour(t), minute(t), day(t), month(t));
  mainMenu->setTitle(title);
  return "VisualTime actualizado.";
}

/*void updateNewAlarm(int id){
  menuAgregar->empty();
  menuAgregar->addItem(alarmaGlobal);
  LCDSubmenu* menu;
  for(int i = 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      char title[MAX_LINE_WIDTH];
      strncpy(title, relayGroups[i].name.c_str(), MAX_LINE_WIDTH);
      menu = new LCDSubmenu(title);
      menu->makeDisposable();
      alarmOnceForms[i+1] = alarmaOcasional->copy(i);
      menu->addItem(alarmOnceForms[i+1]);
      weeklyAlarmForms[i+1] = alarmaSemanal->copy(i);
      menu->addItem(weeklyAlarmForms[i+1]);
      menuAgregar->addItem(menu);
    }
  }
}

void updateAlarmForm(int i){
  RtcDateTime now = Rtc.GetDateTime();
  i++;
  alarmOnceForms[i]->setField("Hora", now.Hour());
  alarmOnceForms[i]->setField("Minutos", now.Minute());
  alarmOnceForms[i]->setField("Hora de fin", now.Hour());
  alarmOnceForms[i]->setField("Minutos de fin", now.Minute());
  alarmOnceForms[i]->setField("Dia", now.Day());
  alarmOnceForms[i]->setField("Mes", now.Month());
  alarmOnceForms[i]->setField("Anio", now.Year());
}

void updateWeeklyAlarmForm(int i){
  RtcDateTime n = Rtc.GetDateTime();
  i++;
  weeklyAlarmForms[i]->setField("Hora", n.Hour());
  weeklyAlarmForms[i]->setField("Minutos", n.Minute());
  weeklyAlarmForms[i]->setField("Hora de fin", n.Hour());
  weeklyAlarmForms[i]->setField("Minutos de fin", n.Minute());
  weeklyAlarmForms[i]->setField("Dia de semana", weekday(now()));
}*/

/*void updateRelayAlarms(int id){
  alarmaRelays->empty();
  LCDSubmenu* menuMaestro = new LCDSubmenu("Maestro");
  alarmaRelays->addItem(menuMaestro);
  LCDForm* aux;
  LCDForm* relayForm = new LCDForm("Relay 1", newRelayAlarm, 1);
  relayForm->addField("Dia", TYPE_INT);
  relayForm->addField("Mes", TYPE_INT);
  relayForm->addField("Anio", TYPE_INT, 2018);
  relayForm->addField("Hora", TYPE_INT);
  relayForm->addField("Minutos", TYPE_INT);
  relayForm->addField("Hora de fin", TYPE_INT);
  relayForm->addField("Minutos de fin", TYPE_INT);
  if(relay1Mode != DISCONNECTED){
    menuMaestro->addItem(relayForm);
  }
  if(relay2Mode != DISCONNECTED){
    aux = new LCDForm("Relay 2", newRelayAlarm, 2);
    relayForm->copyIn(aux);
    menuMaestro->addItem(aux);
  }
  for(int i = 0; i < connectedNodes; i++){
    if((nodes[i].relay1Mode != DISCONNECTED) || (nodes[i].relay2Mode != DISCONNECTED)){
      char* title = (char*)malloc(sizeof(char)*10);
      snprintf(title, 10, "Nodo %d", i);
      LCDSubmenu* menuNodo = new LCDSubmenu(title);
      alarmaRelays->addItem(menuNodo);
      if(nodes[i].relay1Mode != DISCONNECTED){
        aux = new LCDForm("Relay 1", newRelayAlarm, relayNumber(nodes[i], 1));
        relayForm->copyIn(aux);
        menuNodo->addItem(aux);
      }
      if(nodes[i].relay2Mode != DISCONNECTED){
        aux = new LCDForm("Relay 2", newRelayAlarm, relayNumber(nodes[i], 2));
        relayForm->copyIn(aux);
        menuNodo->addItem(aux);
      }
    }
  }
}*/

void updateGroupControls(int id){
  interaccionGrupos->empty();
  for(int i= 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      char title[MAX_LINE_WIDTH];
      LCDFunction* f;
      if(!relayGroups[i].isOn){
        snprintf(title, 20, "Prender %s", relayGroups[i].name.c_str());
        f = new LCDFunction(title, manualTurnOnGroup, i);
      } else {
        snprintf(title, 20, "Apagar %s", relayGroups[i].name.c_str());
        f = new LCDFunction(title, manualTurnOffGroup, i);
      }
      f->makeDisposable();
      interaccionGrupos->addItem(f);
    }
  }
}

void updateRelayGroups(int id){
  menuGrupos->empty();
  for(int i = 2; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      /*char title[MAX_LINE_WIDTH];
      snprintf(title, MAX_LINE_WIDTH, "Eliminar %s", relayGroups[i].name.c_str());
      LCDFunction* f = new LCDFunction(title, deleteRelayGroup, i); f->makeDisposable();
      menuGrupos->addItem(f);*/
      char title[MAX_LINE_WIDTH];
      snprintf(title, MAX_LINE_WIDTH, "Grupo %s", relayGroups[i].name.c_str());
      LCDItem* f = new LCDItem(title); f->makeDisposable();
      menuGrupos->addItem(f);
    }
  }
  //menuGrupos->addItem(crearGrupo);
}
/*
char* newGroupForm(int argc, String argv[], int id){
  for(int i= 2; i < GROUP_AMOUNT; i++){
    if(!relayGroups[i].isConnected){
      relayGroups[i].name = argv[0];
      relayGroups[i].isConnected = true;
      relayGroups[i].isOn = false;
      storeRelayGroupSettings();
      return "Grupo creado.";
    }
  }
  return "Sin espacio para grupos.";
}

char* deleteRelayGroup(int id){
  relayGroups[id].isConnected = false;
  int packet[1];
  for(int i = 0; i < connectedNodes; i++){
    for(int j = 0; j < MAX_SLAVE_RELAY_AMOUNT; j++){
      if(nodes[i]->isRelayConnected(j) && nodes[i]->isRelayInGroup(j, id))
        nodes[i]->disconnectRelay(j);
    }
  }
  storeRelayGroupSettings();
  return "Grupo eliminado.";
}

char* newRelayAlarm(int argc, String argv[], int id){
  addAlarmOnce(argc, argv, id);
  return "Alarma creada.";
}*/

bool getSensorData(int* temp, short int* hum){
  (*temp) = 0;
  (*hum) = 0;
  if(sensors.dht.isConnected){
    return checkTEMPERATURE(temp, hum);
  } else {
    for(int i = 0; i < connectedNodes; i++){
      if(nodes[i]->hasTemperatureSensor()){
        nodes[i]->sendMessage(SENSOR_DATA_MSG);
      }
    }
  }
  return false;
}

char* getVisualSensorData(int id){
  if(sensors.dht.isConnected){
    int temp;
    short int hum;
    if(ambientRead){
      char* text = (char*)malloc(sizeof(char)*40);
      snprintf(text, 40, "Temperatura: %d \rC \nHumedad: %d %% ", temperature, humidity);
      return text;
    }
  }
  for(int i = 0; i < connectedNodes; i++){
    if(nodes[i]->hasTemperatureSensor()){
      nodes[i]->sendMessage(SENSOR_DATA_MSG);
      return "Esperando informacion...";
    }
  }
  return "No hay sensores disponibles.";
}

char* updateSensorData(int id){
  char* text = (char*)malloc(sizeof(char)*40);
  snprintf(text, 40, "Temperatura: %d \rC\nHumedad: %d %%", temperature, humidity);
  return text;
}

void updateControlNodes(int id){
  menuControlar->empty();
  controlarMaestro = new LCDSubmenu("Maestro", updateMasterPins);
  controlarMaestro->makeDisposable();
  menuControlar->addItem(controlarMaestro);
  for(int i = 0; i < connectedNodes; i++){
    char title[MAX_LINE_WIDTH];
    snprintf(title, 10, "Nodo %d", i);
    nodeMenu[i] = new LCDSubmenu(title, updateNodesPins, i);
    nodeMenu[i]->makeDisposable();
    menuControlar->addItem(nodeMenu[i]);
  }
}

void updateNodesPins(int i){
  nodeMenu[i]->empty();
  LCDFunction* f;
  if(!nodes[i]->hasTemperatureSensor()){
    //f = new LCDFunction("Conectar dht", connectTempSensor, i);
    //f->makeDisposable();
    //nodeMenu[i]->addItem(f);
    LCDItem* info = new LCDItem("Dht desconectado.");
    info->makeDisposable();
    nodeMenu[i]->addItem(info);
  } else {
    //f = new LCDFunction("Desconectar dht", disconnectTempSensor, i);
    //f->makeDisposable();
    //nodeMenu[i]->addItem(f);
    LCDItem* info = new LCDItem("Dht conectado.");
    info->makeDisposable();
    nodeMenu[i]->addItem(info);
  }
  if(!nodes[i]->hasPirSensor()){
    //f = new LCDFunction("Conectar PIR", connectPirSensor, i);
    //f->makeDisposable();
    //nodeMenu[i]->addItem(f);
    LCDItem* info = new LCDItem("Pir desconectado.");
    info->makeDisposable();
    nodeMenu[i]->addItem(info);
  } else {
    //f = new LCDFunction("Desconectar PIR", disconnectPirSensor, i);
    //f->makeDisposable();
    //nodeMenu[i]->addItem(f);
    LCDItem* info = new LCDItem("Pir conectado.");
    info->makeDisposable();
    nodeMenu[i]->addItem(info);
  }
  /*for(int j= 0; j < GROUP_AMOUNT; j++){
    if(relayGroups[j].isConnected){
      LCDForm* relayForm;
      char title[MAX_LINE_WIDTH];
      snprintf(title, 20, "Conectar %s", relayGroups[j].name.c_str());
      relayForm = new LCDForm(title, connectRelay, relayNumber(nodes[i], j));
      relayForm->addField("Relay", TYPE_INT, 1, 1, MAX_SLAVE_RELAY_AMOUNT);
      relayForm->makeDisposable();
      nodeMenu[i]->addItem(relayForm);
      snprintf(title, 20, "Desconectar %s", relayGroups[j].name.c_str());
      relayForm = new LCDForm(title, disconnectRelay, relayNumber(nodes[i], j));
      relayForm->addField("Relay", TYPE_INT, 1, 1, MAX_SLAVE_RELAY_AMOUNT);
      relayForm->makeDisposable();
      nodeMenu[i]->addItem(relayForm);
    }
  }*/
}

void updateMasterPins(int id){
  controlarMaestro->empty();
  if(!sensors.dht.isConnected){
    //controlarMaestro->addItem(new LCDFunction("Conectar Dht", visualConnectMasterDht));
    LCDItem* info = new LCDItem("Dht desconectado.");
    info->makeDisposable();
    controlarMaestro->addItem(info);
  } else {
    //controlarMaestro->addItem(new LCDFunction("Desconectar Dht", visualDisconnectMasterDht));
    LCDItem* info = new LCDItem("Dht conectado.");
    info->makeDisposable();
    controlarMaestro->addItem(info);
  }
  if(!sensors.pir.isConnected){
    //controlarMaestro->addItem(new LCDFunction("Conectar Pir", visualConnectMasterPir));
    LCDItem* info = new LCDItem("Pir desconectado.");
    info->makeDisposable();
    controlarMaestro->addItem(info);
  } else{
    //controlarMaestro->addItem(new LCDFunction("Desconectar Pir", visualDisconnectMasterPir));
    LCDItem* info = new LCDItem("Pir conectado.");
    info->makeDisposable();
    controlarMaestro->addItem(info);
  }
  /*LCDForm* relayForm;
  for(int i= 0; i < GROUP_AMOUNT; i++){
    if(relayGroups[i].isConnected){
      char title[MAX_LINE_WIDTH];
      snprintf(title, 20, "Conectar %s", relayGroups[i].name.c_str());
      relayForm = new LCDForm(title, connectMasterRelay, i);
      relayForm->addField("Relay", TYPE_INT, 1, 1, 2);
      relayForm->makeDisposable();
      controlarMaestro->addItem(relayForm);
      snprintf(title, 20, "Desconectar %s", relayGroups[i].name.c_str());
      relayForm = new LCDForm(title, disconnectMasterRelay, i);
      relayForm->addField("Relay", TYPE_INT, 1, 1, 2);
      relayForm->makeDisposable();
      controlarMaestro->addItem(relayForm);
    }
  }*/
}

/*char* visualConnectMasterPir(int id){
  connectMasterPir();
  return "Pir conectado";
}

char* visualConnectMasterDht(int id){
  connectMasterDht();
  return "Dht conectado";
}

char* visualDisconnectMasterPir(int id){
  disconnectMasterPir();
  return "Pir desconectado";
}

char* visualDisconnectMasterDht(int id){
  disconnectMasterDht();
  return "Dht desconectado";
}*/

/*char* connectMasterRelay(int argc, String argv[], int id){
  char* res;
  int relay = argv[0].toInt();
  if(relay == 1){
    if(relay1Mode == DISCONNECTED){
      relay1Mode = id;
      res = "Relay 1 conectado.";
    } else {
      res = "Relay 1 ocupado.";
    }
  } else if(relay == 2){
    if(relay2Mode == DISCONNECTED){
      relay2Mode = id;
      res = "Relay 2 conectado.";
    } else {
      res = "Relay 2 ocupado.";
    }
  }
  storeSensorInfo();
  return res;
}

char* disconnectMasterRelay(int argc, String argv[], int group){
  char* res;
  int relay = argv[0].toInt();
  if(relay == 1){
    if(relay1Mode == group){
      relay1Mode = DISCONNECTED;
      res = "Relay 1 desconectado.";
    } else res = "Relay 1 es de otro grupo.";
  } else if(relay == 2){
    if(relay2Mode == group){
      relay2Mode = DISCONNECTED;
      res = "Relay 2 desconectado.";
    } else res = "Relay 2 es de otro grupo.";
  }
  storeSensorInfo();
  return res;
}*/

bool checkTEMPERATURE(int* temperature, short int* humidity){

  float humi = dht.readHumidity();
  float temp = dht.readTemperature();

  if (isnan(humi) || isnan(temp) ) {
    Serial.println();
    Serial.print("Failed to read from DHT sensor!");
    Serial.println(); 
    return false;
  }else{
    Serial.printf("\n Temperature: ");
    Serial.print(temp);
    Serial.printf("\n Humidity: ");
    Serial.print(humi);
    (*temperature) = static_cast<int>(temp);
    (*humidity) = static_cast<short int>(humi);
    return true;
  }
}

/*char* connectTempSensor(int id){
  nodes[id]->connectTemperatureSensor();
  return "Conectado.";
}

char* disconnectTempSensor(int id){
  nodes[id]->disconnectTemperatureSensor();
  return "Desconectado.";
}

char* connectPirSensor(int id){
  nodes[id]->connectPir();
  return "Conectado.";
}

char* disconnectPirSensor(int id){
  nodes[id]->disconnectPir();
  return "Desconectado.";
}

char* connectRelay(int argc, String argv[], int nodeGroup){
  int relay = argv[0].toInt() - 1;
  int group = nodeGroup % 10;
  int n = nodeFromIp((nodeGroup - group)/10);
  Serial.printf("\n Connecting relay %d from node %d to group %d", relay, n, group);
  nodes[n]->connectRelay(relay, group);
  return "Conectado.";
}

char* disconnectRelay(int argc, String argv[], int nodeGroup){
  int relay = argv[0].toInt() - 1;
  int group = nodeGroup % 10;
  int n = nodeFromIp((nodeGroup - group)/10);
  Serial.printf("\n Disconnecting relay %d from node %d to group %d", relay, n, group);
  if(!nodes[n]->isRelayConnected(relay))
    return "Relay no esta conectado.";
  if(!nodes[n]->isRelayInGroup(relay, group))
    return "Relay de otro grupo.";
  nodes[n]->disconnectRelay(relay);
  return "Desconectado.";
}

char* newAlarmOnce(int argc, String argv[], int id){ 
  addAlarmOnce(argc, argv, id);
  return "Alarma agregada"; 
}

char* newAlarmWeekly(int argc, String argv[], int id){ 
  addWeeklyAlarm(argc, argv, id);
  Serial.printf("\n ID: %d", id);
  return "Alarma agregada"; 
}*/

char* changeTime(int argc, String argv[], int id){ 
  setRTCTime(argc, argv);
  return "Hora configurada.";
}

void updateTimeForm(int id){
  RtcDateTime now = Rtc.GetDateTime();
  ajustarHora->setField("Hora", now.Hour());
  ajustarHora->setField("Minutos", now.Minute());
  ajustarHora->setField("Dia", now.Day());
  ajustarHora->setField("Mes", now.Month());
  ajustarHora->setField("Anio", now.Year());
}

/*char* changeWifiSettings(int argc, String argv[], int id){
  changeWifi(argc, argv); 
  return "Configuracion wifi cambiada."; 
}

void updateWifiForm(int id){
  ajustarWifi->setField("Nombre", readSSID());
  ajustarWifi->setField("Contr.", readPassword());
}*/

void showAlarms(int id){
  listaAlarmas->empty();
  int amount = calendar.alarmNumber();
  alarm* alarms= calendar.getAlarms();
  LCDItem* alarmMenu;
  for(int i = 0; i < amount; i++){
    char title[MAX_LINE_WIDTH];
    switch(alarms[i].type){
      case ALARM_ONCE:
        snprintf(title, 20, "%d/%d/%d %d:%d",alarms[i].day, alarms[i].month, alarms[i].year, alarms[i].hour, alarms[i].minutes);
        break;
      case WEEKLY_ALARM:
        snprintf(title, 20, "%s %d:%d",calendar.days(alarms[i].day), alarms[i].hour, alarms[i].minutes);
        break;
    }
    alarmMenu = new LCDItem(title);
    alarmMenu->makeDisposable();
    listaAlarmas->addItem(alarmMenu);
    //alarmMenu->addItem(new LCDFunction("Eliminar", deleteAlarm, alarms[i].id));
  }
}

/*char* deleteAlarm(int id){
  calendar.removeAlarm(id);
  listaAlarmas->goHome();
  return "Alarma eliminada.";
}*/

char* printAlarm(int id){
  char title[MAX_LINE_WIDTH];
  snprintf(title, MAX_LINE_WIDTH, "Alarma %d", id);
  return title;
}

/*char* visualNewAlarm(int argc, String argv[]){
  for(int i = 0; i < argc; i++){
    Serial.printf("\n      %s", argv[i].c_str());
  }
  return "Enviado";
}*/

char* visualReset(int id){
  delay(2000);
  ESP.reset();
  return "Vistual reset";
}

void visualInitNodes(int id){
  visualUpdateNodes(id);
}

char* visualUpdateNodes(int id){
  menuEstado->empty();
  for(int i = 0; i < connectedNodes; i++){
    char title[MAX_LINE_WIDTH];
    char state[MAX_LINE_WIDTH];
    printState(nodes[i], state);
    snprintf_P(title, MAX_LINE_WIDTH, "Nodo %d: %s", i, state);
    LCDItem* node = new LCDItem(title);
    node->makeDisposable();
    menuEstado->addItem(node);
  }
  return "";
}

void printState(Node* node, char state[]){
  switch(node->getState()){
    case PENDING_STATE: strncpy(state,"No responde", MAX_LINE_WIDTH); break;
    case RUNNING_STATE: strncpy(state,"Activo", MAX_LINE_WIDTH); break;
    case PENDING_RESPONSE_STATE: strncpy(state,"No responde", MAX_LINE_WIDTH); break;
    case ERROR_STATE: strncpy(state, "Error", MAX_LINE_WIDTH); break;
  }
}
