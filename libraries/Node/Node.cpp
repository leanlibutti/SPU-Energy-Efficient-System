#include <Node.h>

Node::Node(WiFiUDP& socket, IPAddress newIp):
  pendingPackets(Queue(sizeof(Packet), 15, FIFO,false)),
  udp(socket){
  ip = IPAddress(newIp);
  lastMessageNumber = 0;
  pirSensorConnected = false;
  temperatureSensorConnected = false;
  currentSensorConnected = false;
  irControlConnected = false;
  timeSinceSent = 0;
  state = RUNNING_STATE;
}

// Getters y setters
IPAddress Node::getIp(){
  return IPAddress(ip);
}

int Node::getState(){
  return state;
}

void Node::setIp(IPAddress newIp){
  ip = newIp;
}

bool Node::hasTemperatureSensor(){
  return temperatureSensorConnected;
}

bool Node::hasPirSensor(){
  return pirSensorConnected;
}

bool Node::hasCurrentSensor(){
  return currentSensorConnected;
}

bool Node::hasIrControl(){
  return irControlConnected;
}

bool Node::hasPendingState(){
  return state == PENDING_STATE;
}

bool Node::hasPendingResponse(){
  return state == PENDING_RESPONSE_STATE;
}

bool Node::hasRunningState(){
  return state == RUNNING_STATE;
}

bool Node::hasErrorState(){
  return state == ERROR_STATE;
}

void Node::error(){
  state = ERROR_STATE;
}

// Envia un mensaje a un nodo
// junto con numero de mensaje, que se incrementa
void Node::sendMessage(int code) {
  Packet p;
  state = PENDING_RESPONSE_STATE;
  p.payload[0] = code;
  p.payload[1] = lastMessageNumber++;
  p.length = 2;
  pendingPackets.push(&p);
  if(pendingPackets.getCount() == 1)
    sendNextPacket();
}

// Envia un paquete a un nodo (lo pone en la cola para que se envien en orden)
// junto con numero de mensaje, que se incrementa
void Node::sendPacket(int code, int* packet, int len) {
  Packet p;
  state = PENDING_RESPONSE_STATE;
  p.payload[0] = code;
  for(int i = 0;i < len; i++) p.payload[i+1] = packet[i];
  p.payload[len+1] = lastMessageNumber++;
  p.length = len+2;
  if(pendingPackets.getCount() == 15) Serial.printf("\n Queue full");
  pendingPackets.push(&p);
  if(pendingPackets.getCount() == 1)
    sendNextPacket();
}

// Envia el siguiente paquete en la cola, si es que lo hay
void Node::sendNextPacket(){
  if(pendingPackets.isEmpty()){
    state = RUNNING_STATE;
    timeSinceSent = 0;
  } else {
    Packet p;
    pendingPackets.peek(&p);
    udp.beginPacket(ip, DEFAULT_PORT);
    for(int i = 0; i < p.length; i++) udp.write((byte) p.payload[i]);
    udp.endPacket();
  }
}

// Envia mensaje de respuesta al nodo
void Node::sendAck(){
  udp.beginPacket(ip, DEFAULT_PORT);
  udp.write((byte) RECEIVED_MSG);
  udp.write((byte) 0);
  udp.endPacket();
}

// Indica que se recibio la respuesta del ultimo mensaje de la cola,
// por lo que se trata de enviar el siguiente
void Node::receiveAck(){
  Packet p;
  pendingPackets.pop(&p);
  sendNextPacket();
  timeSinceSent = 0;
}

void Node::clearMessages(){
  Serial.printf("\n Clearing pending messages for node %d", ip[3]);
  pendingPackets.clean();
}

// Controla cuanto tiempo pasa sin recibir la respuesta de un mensaje,
// lo reenvia de ser necesario y luego de un tiempo sin respuesta
// se indica que el nodo no tiene conexión
void Node::checkSentPackets(){
  if(!pendingPackets.isEmpty()){
    Serial.printf("\n Node %s not responding for %d", ip.toString().c_str(), timeSinceSent);
    if(timeSinceSent > 0){
      if((timeSinceSent < 5) ||
        ((timeSinceSent < 16) && (timeSinceSent % 2 == 0))){
      //if((timeSinceSent < 16) && (timeSinceSent % 2 == 0)){
        Serial.printf("\n Resending last message for node %s", ip.toString().c_str());
        sendNextPacket();
      } else if(timeSinceSent >= 16){
        state = ERROR_STATE;
        Serial.printf("\n Node %s is disconnected.", ip.toString().c_str());
      }
    }
    timeSinceSent++;
  }
}

// Recibe la informaciond de los sensores y relays del nodo y la almacena
void Node::handleSensorInfo(bool tempConnected, bool pirConnected, bool irConnected, relayInfo* r, int irGroup){
  temperatureSensorConnected = tempConnected;
  pirSensorConnected = pirConnected;
  irControlConnected = irConnected;
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    relays[i] = r[i];
  }
}

// Retorna si el relay esta en uso
bool Node::isRelayConnected(int relay){
  return relays[relay].connected;
}

// Retorna si el relay pertence al grupo
bool Node::isRelayInGroup(int relay, int group){
  return relays[relay].connected && (relays[relay].group == group);
}

void Node::disconnectFromGroup(int group){
  for(int i = 0; i < MAX_SLAVE_RELAY_AMOUNT; i++){
    if(isRelayConnected(i) && isRelayInGroup(i, group))
      disconnectRelay(i);
  }
  if(hasIrControl() && (irControlGroup == group))
    disconnectIrControl();
}

// Metodos de conexion y desconexion de sensores y relays
void Node::connectRelay(int relay, int group){
  int packet[] = {relay, group};
  sendPacket(CONNECT_RELAY_MSG, packet, 2);
  relays[relay].connected = true;
  relays[relay].group = group;
}

void Node::disconnectRelay(int relay){
  int packet[] = {relay};
  sendPacket(DISCONNECT_RELAY_MSG, packet, 2);
  relays[relay].connected = false;
}

void Node::connectPir(){
  sendMessage(CONNECT_PIR_MSG);
  pirSensorConnected = true;
}

void Node::disconnectPir(){
  sendMessage(DISCONNECT_PIR_MSG);
  pirSensorConnected = false;
}

void Node::connectTemperatureSensor(){
  sendMessage(CONNECT_TEMPERATURE_MSG);
  temperatureSensorConnected = true;
}

void Node::disconnectTemperatureSensor(){
  sendMessage(DISCONNECT_TEMPERATURE_MSG);
  temperatureSensorConnected = false;
}

void Node::connectIrControl(int group){
  int packet[] = {group};
  sendPacket(CONNECT_IR_CONTROL_MSG, packet, 1);
  irControlConnected = true;
  irControlGroup = group;
}

void Node::disconnectIrControl(){
  sendMessage(DISCONNECT_IR_CONTROL_MSG);
  irControlConnected = false;
}

// Se llama para indicar que el nodo se reinició y conecto nuevamente
void Node::reconnect(){
  state = RUNNING_STATE;
  lastMessageNumber = 1;
  timeSinceSent = 0;
  sendAck();
}