#include <Map.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <SPU.h>
#include <SerialMenu.h>
#include <IRControl.h>
#include <math.h>
#include "DHT.h"
#include <LEDIndicator.h>
#define DHTTYPE DHT22

// Pines predeterminados para sensores
#define RELAY1_PIN D3
#define RELAY2_PIN D4
#define PIR_PIN D2
#define CURRENT_PIN A0 
#define TEMPERATURE_PIN D1
#define IR_SEND_PIN D8
#define EEPROM_PIN D7
#define LED_RED_PIN 12
#define LED_GREEN_PIN 14

DHT dht(TEMPERATURE_PIN, DHTTYPE);
// Tiempo entre cada lectura de los sensores
#define SENSORS_UPDATE_TIME 5

// Tiempo para reenviar el mensaje si no hubo respuesta del maestro
#define RESEND_TIME 5

// Variables de la conexion
IPAddress masterIp(192, 168, 0, 199); // Ip del nodo maestro
IPAddress nodeIp; // Ip del esclavo, otorgada por el maestro
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 1);

WiFiUDP udp;

// Alarma para reenviar mensaje a master en caso de no haber respuesta
int resendAlarm = ALARM_NOT_SET;

// Variables para envio de mensajess
int receivedMsgNumber = 1; //Último número de mensaje recibido desde master
int sentMsgNumber = 0; //Último número de mensaje enviado a master
int sentMsg; //Último mensaje enviado a master
int sentPacket[MAX_PACKET_SIZE]; // Último paquete enviado a master
int sentPacketSize;
int sentPacketCode;
bool packetSent;
int masterState = RUNNING_STATE; // Estado del maestro, no se si es necesario
int timeSinceConnection = 0;

// Variables de sensores
relayInfo relays[MAX_SLAVE_RELAY_AMOUNT];
bool lightsOn = false;
int pirPin = DISCONNECTED;
int currentSensorPin = DISCONNECTED;
int temperatureSensorPin = DISCONNECTED;
int irControlPin = DISCONNECTED;
int irControlGroup = 0;
volatile int pirState; // Estado del PIR
float sensibility = 0.172; //sensibilidad en Volt/Amper para sensor de 5A
const int phicosine = 1; // desfasaje

IRControl ircontrol(IR_SEND_PIN);
// Variables para recibir codigo IR
int irCodeLength = 0;
int irReceivingLength = 0;
unsigned short irCode[IR_CODE_LENGTH];

// Datos de sensores
int temperature;
short int humidity;
float current;

double lastPirDetection = 0;

Menu menu;
LEDIndicator led(LED_RED_PIN, LED_GREEN_PIN);

void sendMessage(int code, bool resend = true);

void setup() {
  Serial.begin(115200);
  for(int i = 0; i < 50; i++) Serial.println();
  udp.begin(4000);
  EEPROM.begin(MEMORY_SIZE);
  setTime(0, 0, 0, 1, 1, 2000);
  ircontrol.begin();
  initMenu();
  Alarm.timerRepeat(1, serialEvent);
  initSensors(); //Inicializa sensores
  initConnection();

  Alarm.timerRepeat(STATE_CHECK_TIME, checkMasterStatus);
}

void loop() { // Espera paquetes, controla la conexion y el pir
  checkConnection();
  if(udp.parsePacket()){
    handlePacket(); 
  }
  if(pirPin != DISCONNECTED){
    pirState = digitalRead(PIR_PIN);
  }
  checkPIR();
  checkEeprom();
  led.update();
  Alarm.delay(0);
}

// Recibe el paquete y decide que hacer
// Aca deberiamos decidir cuando es un mensaje (longitud de paquete = 2)
// o cuando es un paquete de datos (longitud > 2) y hacer algo al respecto
void handlePacket(){
  byte incomingPacket[MAX_PACKET_SIZE];
  int len = udp.read(incomingPacket, MAX_PACKET_SIZE);
  timeSinceConnection = 0;
  if (len == 2){
    handleMessage((int)incomingPacket[0]);
    receivedMsgNumber = (int)incomingPacket[1];
  } else {
    handleData(incomingPacket, len);
  }
} 

// Maneja los mensajes
// Si llega un mensaje pero todavía se esta esperando respuesta, se ignora? se reenvia el mensaje anterior?
void handleMessage(int code){
  printTime();
  //Serial.printf("\n Message: %d", code);
  if(masterState == PENDING_RESPONSE_STATE){
    masterState = RUNNING_STATE;
    Alarm.free(resendAlarm);
    resendAlarm = ALARM_NOT_SET;
    if(code == RECEIVED_MSG) return;
  }
  switch(code){
    case STATE_MSG:
      Serial.printf("\nSending state to master");
      sendMessage(RUNNING_STATE, false);
      break;
    case CONNECT_PIR_MSG:
      initPir();
      writeSensors();
      Serial.printf("\nPIR on");
      sendAck();
      break;
    case DISCONNECT_PIR_MSG:
      disconnectPir();
      writeSensors();
      //deleteActiveSensor(2,8);
      Serial.printf("\nPIR off");
      sendAck();
      break;
    case CONNECT_TEMPERATURE_MSG:
      initTemperatureSensor();
      writeSensors();
      //storeActiveSensor(3,9);
      Serial.printf("\nTemperature sensor on");
      sendAck();
      break;
    case DISCONNECT_TEMPERATURE_MSG:
      disconnectTemperatureSensor();
      writeSensors();
      //deleteActiveSensor(3,10);
      Serial.printf("\nTemperature sensor off");
      sendAck();
      break;
    case DISCONNECT_IR_CONTROL_MSG:
      Serial.printf("\n Disconnecting ir control");
      disconnectIrControl();
      sendAck();
      break;
    case SENSOR_DATA_MSG:
      Serial.printf("\n Sending sensor data to master");
      sendSensorData();
      break;
    case SENSOR_INFO_MSG:
      Serial.printf("\n Sending sensor info to master");
      sendSensorInfo();
      break;
    case RECEIVED_MSG:
      Serial.printf("\n Ack received.");
      break;
    default:
      Serial.printf("\nUnknown message code received: '%i'", code);
      sendAck();
      break;
  }
}

// Maneja los paquetes de datos que llegan
void handleData(byte packet[], int length){
  int pos = 0;
  int code = (int)packet[pos++];
  if(masterState == PENDING_RESPONSE_STATE){
    masterState = RUNNING_STATE;
    Alarm.free(resendAlarm);
    resendAlarm = ALARM_NOT_SET;
    if(code == RECEIVED_MSG) return;
  }
  switch(code){
    case IP_MSG: {
      sendAck();
      masterState = RUNNING_STATE;
      timeSinceConnection = 0;
      Alarm.free(resendAlarm);
      resendAlarm = ALARM_NOT_SET;
      IPAddress newIp((int)packet[1], (int)packet[2], (int)packet[3], (int)packet[4]);
      nodeIp = newIp;
      storeIp(nodeIp);
      Serial.printf("\nNew IP: %s", nodeIp.toString().c_str());
      WiFi.disconnect(true);
      initConnection(); // Reinicia la conexion con la ip nueva
      break;}
    case WIFI_SETTINGS_MSG: {
      sendAck();
      char* ssid = (char*)malloc(MAX_SSID_LENGTH*sizeof(char));
      char* passwd = (char*)malloc(MAX_PASSWD_LENGTH*sizeof(char));
      int i = 0;
      int j = 0;
      while(packet[pos]){
        ssid[i++]= (char)packet[pos++];
      }
      ssid[i++] = '\0';
      pos++;
      while(packet[pos]){
        passwd[j++]= (char)packet[pos++];
      }
      passwd[j++] = '\0';
      Serial.printf("\n SSID: %s  Pass: %s", ssid, passwd);
      writeSSID(ssid);
      writePassword(passwd);
      sendMessage(IP_MSG);
      break;}
    case TURN_ON_GROUP_MSG:{
      sendAck();
      int group = (int)packet[pos];
      turnOnGroup(group);}
      break;
    case TURN_OFF_GROUP_MSG:{
      sendAck();
      int group = (int)packet[pos];
      turnOffGroup(group);
      break;}
    case CONNECT_RELAY_MSG:{
      sendAck();
      int relay = (int)packet[pos++];
      int group = (int)packet[pos++];
      Serial.printf("\n Connecting relay %d  to group %d", relay, group);
      connectRelay(relay, group);
      break;}
    case DISCONNECT_RELAY_MSG:{
      sendAck();
      int relay = (int)packet[pos++];
      Serial.printf("\n Disconnecting relay %d ", relay);
      disconnectRelay(relay);
      break;}
    case CONNECT_IR_CONTROL_MSG:{
      sendAck();
      int group = (int)packet[pos++];
      Serial.printf("\n Connecting ir control to group %d", group);
      connectIrControl(group);
      break;}
    case IR_LENGTH_MSG:{
      sendAck();
      byte v[2] = {packet[pos], packet[pos+1]};
      memcpy(&irReceivingLength, &v, 2);
      irCodeLength = 0;
      Serial.printf("\n Code length: %d", irReceivingLength);
      break;}
    case IR_CODE_MSG:
      sendAck();
      for(pos; pos < length-1; pos+= 2){
        byte v[2] = {packet[pos], packet[pos+1]};
        memcpy(&irCode[irCodeLength++], &v, 2);
        Serial.printf("\nPos %d value %d: %d", pos, irCodeLength-1, irCode[irCodeLength-1]);
        if(irCodeLength == irReceivingLength){
          ircontrol.storeCode(irCode, irCodeLength);
          irCodeLength = 0;
          break;
        }
      }
      break;
    default:
      sendAck();
      Serial.printf("\n Unknown packet received from master. Code: %d", code);
      break;
  }
}

// Realiza la conexion con el maestro
void connectToMaster(){
  int message;
  Serial.printf("\nConnecting to %s\nSending first request\n", masterIp.toString().c_str());
  sendMessage(CONNECTION_MSG);
  Serial.printf("\nWaiting for response");
  int i = 0;
  led.blinkGreen();
  while(!udp.parsePacket()){
    Serial.printf(".");
    checkEeprom();
    delay(500);
    led.update();
    i++;
    if(i == 10) reset();
    else if(i == 5) sendMessage(CONNECTION_MSG);
  }
  byte incomingPacket[MAX_PACKET_SIZE];
  int len = udp.read(incomingPacket, MAX_PACKET_SIZE);
  message = (int)incomingPacket[0];
  if(message == RECEIVED_MSG){
    masterState = RUNNING_STATE;
    Serial.printf("\nConected to master");
    led.stillGreen();
    return;
  }
}

// Conecta a la red del maestro, configurando antes la ip estatica
void connectWifi(IPAddress ip, bool paired){
  WiFi.config(ip, gateway, subnet);
  connectWifi(paired);
}

// Conecta a la red del maestro por DHCP
void connectWifi(bool paired){
  Alarm.free(resendAlarm);
  resendAlarm = ALARM_NOT_SET;
  WiFi.mode(WIFI_STA);
  const char* ssid = readSSID();
  const char* passwd = readPassword();
  if(!paired)
    led.blinkRed();
  else
    led.alternate();
  WiFi.begin(ssid, passwd);
  Serial.printf("\n Connecting to '%s' with password '%s'", ssid, passwd);
  int i = 0;
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Alarm.delay(0);
    Serial.print(".");
    led.update();
    checkEeprom();
    i++;
    if(i == 30) {
      reset();
    }  
  }
  Serial.printf("\n Wifi connected.");
}

// Envía un mensaje de respuesta
bool sendAck(){
  noInterrupts();
  udp.beginPacket(masterIp, DEFAULT_PORT);
  udp.write((byte)RECEIVED_MSG);
  udp.write((byte)sentMsgNumber);
  bool result = udp.endPacket();
  //yield(); En el compilador 2.6.3 crashea por esto, no me doy cuenta porque
  sentMsgNumber++;
  sentMsg = RECEIVED_MSG;
  interrupts();
  return result;
}

// Controla que la conexion no se haya perdido y la restaura
void checkConnection(){
  if(WiFi.status() != WL_CONNECTED){
    Serial.printf("\nConnection lost.");
    WiFi.reconnect();
    int i= 0;
    led.alternate();
    while(WiFi.status() != WL_CONNECTED){
      delay(1000);
      Serial.print(".");
      checkEeprom();
      led.update();
      i++;
      if(i == 20) reset();
    }
    connectToMaster();
    sendSensorInfo();
    Serial.printf("\nConnection restored.");
  }
}

// Controla el tiempo sin mensajes del maestro y vuelve a conectarse a la red de ser necesario
void checkMasterStatus(){
  printTime();
  if(timeSinceConnection >= STATE_CHECK_TIME*2){
    Serial.printf("\n Master disconnected. Reconnecting to wifi");
    WiFi.disconnect();
    delay(500);
    connectWifi(storedIp(), true);
    connectToMaster();
    sendSensorInfo();
    timeSinceConnection = 0;
  }
  timeSinceConnection += STATE_CHECK_TIME;
}

// Conecta con el maestro y pide IP si es necesario
void initConnection(){
  nodeIp = storedIp();
  if(nodeIp[0] != 0){
    Serial.printf("\nIP: %s", nodeIp.toString().c_str());
    connectWifi(nodeIp, true);
    connectToMaster();
    sendSensorInfo();
  } else {
    connectWifi(false);
    connectToMaster();
    sendMessage(WIFI_SETTINGS_MSG);
    Serial.printf("\nRequesting new Wifi settings.");
  }
}

// Envía la informacion de los sensores al maestro
void sendSensorData(){
  int packet[MAX_PACKET_SIZE];
  int length = 0;
  if(temperatureSensorPin != DISCONNECTED){
    packet[length] = temperature;
    length++;
    packet[length] = humidity;
    length++;
  }
  if(currentSensorPin != DISCONNECTED){
    unsigned short int c = static_cast<unsigned short int>(current*1000);
    packet[length] = c >> 8;
    length++;
    packet[length] = c - (c >> 8);
    length++;
  }
  sendPacket(SENSOR_DATA_MSG, packet, length);
}

// Envía que sensores están conectados al maestro
// Primer byte es info de sensores
// Despues grupo de cada relay
void sendSensorInfo(){
  int packet[MAX_SLAVE_RELAY_AMOUNT+2];
  packet[0] = sensorInfoByte();
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    packet[i+1] = relays[i].group;
  }
  packet[MAX_SLAVE_RELAY_AMOUNT+1] = irControlGroup;
  sendPacket(SENSOR_INFO_MSG, packet, MAX_SLAVE_RELAY_AMOUNT+2);
  setResendAlarm();
}

// Lee la ip almacenada
IPAddress storedIp(){
  int n[4];
  for(int i = 0; i <= 3; i++){
    n[i]= (int)EEPROM.read(IP_DIR+i);
  }
  IPAddress ip(n[0], n[1], n[2], n[3]);
  return ip;
}

// Almacena la ip nueva
void storeIp(IPAddress ip){
  for(int i= 0; i < 4; i++){
    EEPROM.write(i+IP_DIR, ip[i]);
  }
  EEPROM.commit();
}

// Envia un mensaje, incrementa el numero de mensaje
void sendMessage(int code, bool resend){
  udp.beginPacket(masterIp, DEFAULT_PORT);
  udp.write((byte)code);
  udp.write((byte)sentMsgNumber);
  udp.endPacket();
  yield();
  sentMsgNumber++;
  sentMsg = code;
  packetSent = false;
  if(resend)
    setResendAlarm();
}

// Envia un paquete de datos
int sendPacket(int code, int* packet, int len) {
  Serial.printf("\n Sending packet %d. length: %d", code, len);
  udp.beginPacket(masterIp, DEFAULT_PORT);
  udp.write((byte)code);
  for(int i = 0; i < len; i++){
    udp.write((byte)packet[i]);  
  }
  udp.write((byte)sentMsgNumber);
  if(len == 1){ // Si el paquete es muy pequeño se llena para que el maestro lo identifique como paquete
    udp.write((byte)0);
  }
  int result = udp.endPacket();
  yield();
  sentMsgNumber++;
  for(int i = 0; i < MAX_PACKET_SIZE; i++){
    sentPacket[i] = packet[i]; 
  }
  packetSent = true;
  sentPacketSize = len;
  sentPacketCode = code;
  return result;
}

void setResendAlarm(){
  masterState = PENDING_RESPONSE_STATE;
  resendAlarm = Alarm.timerOnce(RESEND_TIME, resendLastMessage);
}

void resendLastMessage(){
  if(packetSent){
    Serial.printf("\n Resending last packet %d", sentPacketCode);
    sendPacket(sentPacketCode, sentPacket, sentPacketSize);
  } else {
    Serial.printf("\n Resending last message %d", sentMsg);
    sendMessage(sentMsg);
  }
}

// Notifica que se detectó movimiento
void detectMotion(){
  pirState = HIGH;
}

// Hace cualquier checkeo necesario para sensores
void checkSensors(){
  if(currentSensorPin != DISCONNECTED){
    checkCURRENT();
  }
  if(temperatureSensorPin != DISCONNECTED){
    checkTEMPERATURE();
  }
}

// Si se detectó movimiento, envia mensaje al maestro
void checkPIR(){
  if((pirPin != DISCONNECTED) && (pirState == HIGH) && ((millis() - lastPirDetection) > 500)){
    sendMessage(MOTION_DETECTED_MSG);
    printTime();
    Serial.printf("\nMotion detected");
    pirState = LOW;
    lastPirDetection = millis();
  }
}

// Inicializa el PIR
void initPir(){
  pirPin = PIR_PIN;
  pirState = digitalRead(PIR_PIN);
  //attachInterrupt(digitalPinToInterrupt(PIR_PIN), detectMotion, FALLING);
}

// 'Desconecta' el PIR
void disconnectPir(){
  pirPin = DISCONNECTED;
  detachInterrupt(PIR_PIN);
}

// Inicializa el sensor de temperatura
void initTemperatureSensor(){
  temperatureSensorPin = TEMPERATURE_PIN;
}

// desconecta el sensor de temperatura
void disconnectTemperatureSensor(){
  temperatureSensorPin = DISCONNECTED;
}

//inicializa el sensor de corriente
void initCurrentSensor(){
  currentSensorPin = CURRENT_PIN;
}

// desconecta el sensor de corriente
void disconnectCurrentSensor(){
  currentSensorPin = DISCONNECTED;
}

// Inicializa el relay que se pasa por argumento
void connectRelay(int relay, int group){
  relays[relay].connected = true;
  relays[relay].group = group;
  writeSensors();
}

// Desconecta el relay que se pasa por argumento
void disconnectRelay(int relay){
  relays[relay].connected = false;
  digitalWrite(relays[relay].pin, HIGH);
  writeSensors();
}

void connectIrControl(int group){
  irControlPin = IR_SEND_PIN;
  irControlGroup = group;
  writeSensors();
}

void disconnectIrControl(){
  irControlPin = DISCONNECTED;
  writeSensors();
}

// Prende un grupo de relays
void turnOnGroup(int group){
  Serial.printf("\n Prender grupo %d", group);
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(relays[i].connected && (relays[i].group == group)){
      Serial.printf("\n Prender relay %d", i);
      digitalWrite(relays[i].pin, HIGH);
    }
  }
}

// Apaga un grupo de relays
void turnOffGroup(int group){
  Serial.printf("Apagar grupo %d", group);
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(relays[i].connected && (relays[i].group == group)){
      Serial.printf("\n Apagar relay %d", i);
      digitalWrite(relays[i].pin, LOW);
    }
  }
  if((irControlPin != DISCONNECTED) && (irControlGroup == group))
    ircontrol.sendCode();
}

// Inicializa los sensores
void initSensors(){
  relays[0].pin = RELAY1_PIN;
  relays[1].pin = RELAY2_PIN;
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT_PULLUP);
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  dht.begin();
  Alarm.timerRepeat(SENSORS_UPDATE_TIME, checkSensors);
  readSensors();
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(relays[i].connected){
      pinMode(relays[i].pin, OUTPUT);
      digitalWrite(relays[i].pin, HIGH);
    }
  }
}

// Lee los sensores conectados en la EEPROM
void readSensors(){
  bool b[8];
  int val = EEPROM.read(SENSOR_DIR);
  for(int i= 7; i >= 0; i--){
    b[i]= (val % 2 == 1);
    val/=2;
  }
  if(b[PIR_DIR]) initPir();
  if(b[TEMP_DIR]) initTemperatureSensor();
  if(b[IR_DIR]){
   irControlPin = IR_SEND_PIN; 
   irControlGroup = EEPROM.read(IR_CONTROL_GROUP_DIR);
  }
  for(int j = 0; j < MAX_SLAVE_RELAY_AMOUNT; j++){
    if(b[RELAY_DIR+j]){
      relays[j].connected = true;
      Serial.printf("\n Relay %d conectado", j);
    }
  }
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(relays[i].connected){
      relays[i].group = EEPROM.read(RELAY_GROUP_DIR+i);
    }
  }
}

void writeSensors(){
  int val = sensorInfoByte();
  Serial.printf("\n info: %d", val);
  EEPROM.write(SENSOR_DIR, val);
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(relays[i].connected){
      EEPROM.write(RELAY_GROUP_DIR+i, relays[i].group);
    } else {
      EEPROM.write(RELAY_GROUP_DIR+i, 0);
    }
  }
  EEPROM.write(IR_CONTROL_GROUP_DIR, irControlGroup);
  EEPROM.commit();
}

int sensorInfoByte(){
  bool b[8];
  for(int i = 0; i < 8; i++) b[i]= false;
  if(pirPin != DISCONNECTED) b[PIR_DIR] = true;
  if(temperatureSensorPin != DISCONNECTED) b[TEMP_DIR] = true;
  if(irControlPin != DISCONNECTED) b[IR_DIR] = true;
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(relays[i].connected){
      b[i+RELAY_DIR] = true;
    }
  }
  int val = 0;
  for(int j= 0; j < 8; j++){
    val*=2;
    val+= (b[j])?1:0;
  }
  return val;
}


void checkCURRENT(){
  float corriente = get_corriente(200); // obtengo la corriente promedio de 200 muestras 
  Serial.printf("\n Corriente: ");
  Serial.print(corriente,3); // 3 digitos como maximo
  current = corriente;
}

// ------- CORRIENTE ALTERNA --------
float get_corriente(int n_sample){
  float voltageSensor;
  float current = 0; // reseteo la corriente para calcular entre las muestras
  float maximum = 0; // reseteo el maximo
  for(int i=0;i<n_sample;i++){
    voltageSensor = analogRead(A0)*(5.0/1023.0); //lectura del sensor    
    current=(voltageSensor-2.5)/sensibility; //Ecuación  para obtener la corriente
    if (current > maximum){ // calculo el valor pico
      maximum = current;
    }
  }
  current = maximum/1.41; // valor eficaz
  current = current*phicosine; 
  current = current+0.15; // factor de ajuste
  return(current); // corriente en amper
}

void checkTEMPERATURE(){

  float humi = dht.readHumidity();
  float temp = dht.readTemperature();

  if (isnan(humi) || isnan(temp) ) {
    Serial.println();
    Serial.print("Failed to read from DHT sensor!");
    Serial.println();  
  }else{
    Serial.printf("\n Temperature: ");
    Serial.print(temp);
    Serial.printf("\n Humidity: ");
    Serial.print(humi);
    temperature = static_cast<int>(temp);
    humidity = static_cast<short int>(humi);
  }
}

void printTime(){
  // digital clock display of the time
  time_t t = now();
  Serial.println(); 
  Serial.print(hour(t));
  printDigits(minute(t));
  printDigits(second(t));
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void reset(){
  Serial.printf("\nRESTARTING");
  delay(2000);
  ESP.reset();
}

void serialEvent(){
  menu.serialEvent();
}

// Crea el menu serial
void initMenu(){
  menu.addEvent("EVENTS", printEvents);
  menu.addEvent("CLEAR_EEPROM", clearEEPROM);
  menu.addEvent("PRINT_EEPROM", printEprom);
  menu.addEvent("wifi_settings", changeWifi);
  menu.addEvent("wifi_ssid", changeSSID);
  menu.addEvent("wifi_passwd", changePassword);
  menu.addEvent("connectRelay", cr);
  menu.addEvent("disconnectRelay", dcr);
  menu.addEvent("turnongroup", tog);
  menu.addEvent("turnoffgroup", toffg);
  menu.addEvent("Relays", showRelays);
  menu.addEvent("reset", reset);
  menu.addDefault(defaultEvent);
}

void showRelays(int argc, String argv[]){
  Serial.printf("\n RELAYS:");
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(relays[i].connected)
      Serial.printf("\n Relay %d Grupo %d", i, relays[i].group);
  }
}

void cr(int argc, String argv[]){
  connectRelay(argv[0].toInt(), argv[1].toInt());
}

void dcr(int argc, String argv[]){
  disconnectRelay(argv[0].toInt());
}

void tog(int argc, String argv[]){
  turnOnGroup(argv[0].toInt());
}

void toffg(int argc, String argv[]){
  turnOffGroup(argv[0].toInt());
}

void printEvents(int argc, String args[]){
  menu.printEvents();
}

void clearEEPROM(int argc, String args[]){
  for (int i = 0; i < MEMORY_SIZE; i++)
    EEPROM.write(i, 0);
  EEPROM.commit();
  Serial.printf("\n EEPROM clear.");
}

void checkEeprom(){
  if(digitalRead(EEPROM_PIN) == LOW){
    String* aux = NULL;
    clearEEPROM(0, NULL);
  }
}

void printEprom(int argc, String args[]){
  for(int i = 0; i < 71; i++){
    Serial.printf("\nPosition %d: %d", i, EEPROM.read(i));  
  }  
}

void defaultEvent(int argc, String args[]){
  Serial.printf("\n Unknown command: '%s'", args[0].c_str());
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


// En caso de que sea necesario ingresar ssid y contraseña manualmente
void changeWifi(int argc, String argv[]){
  if(argc < 2){
    Serial.printf("\n Please enter new SSID (%d characters max) \n and new password (%d characters max, %d min)",
      MAX_SSID_LENGTH, MAX_PASSWD_LENGTH, MIN_PASSWD_LENGTH);
  } else {
    String aux[] = {argv[1]};
    changeSSID(1, argv);
    changePassword(1, aux);
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

void reset(int argc, String args[]){
  ESP.reset();
}