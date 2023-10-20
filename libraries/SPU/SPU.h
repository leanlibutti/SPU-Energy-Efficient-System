#ifndef SPU_H
#define SPU_H

#define STATE_MSG 0
#define RECEIVED_MSG 255
#define CONNECTION_MSG 254
#define IP_MSG 249

#define MOTION_DETECTED_MSG 2
#define MOTION_ENDED_MSG 3
#define CONNECT_PIR_MSG 7
#define DISCONNECT_PIR_MSG 8
#define CONNECT_CURRENT_MSG 9
#define DISCONNECT_CURRENT_MSG 10
#define CONNECT_TEMPERATURE_MSG 11
#define DISCONNECT_TEMPERATURE_MSG 12
#define CONNECT_IR_CONTROL_MSG 22
#define DISCONNECT_IR_CONTROL_MSG 23
#define CONNECT_RELAY_MSG 14
#define DISCONNECT_RELAY_MSG 15
#define SENSOR_DATA_MSG 13
#define SENSOR_INFO_MSG 18
#define WIFI_SETTINGS_MSG 19
#define TURN_ON_GROUP_MSG 20
#define TURN_OFF_GROUP_MSG 21
#define IR_CODE_MSG 24
#define IR_LENGTH_MSG 25

#define PENDING_STATE 253
#define RUNNING_STATE 252
#define ERROR_STATE 251
#define PENDING_RESPONSE_STATE 250

#define MAX_PACKET_SIZE 100
#define ALARM_NOT_SET -1 //Valor para identificar alarma no seteada

#define DISCONNECTED -1
#define STATE_CHECK_TIME 15 // Tiempo para checkear el estado de los nodos

#define MEMORY_SIZE 4096

#define MAX_MASTER_RELAY_AMOUNT 2
#define MAX_SLAVE_RELAY_AMOUNT 2

#define GROUP_NAME_LENGTH 15
#define MOTION_DET_TIME_LENGTH 2 // unsigned short, 2 bytes

#define IR_CODE_LENGTH 220
#define IR_VALUE_LENGTH 2 // bytes
#define IR_CODE_DIR (MEMORY_SIZE - (IR_CODE_LENGTH*IR_VALUE_LENGTH))

// Longitud de datos en el maestro
#define MAX_IP_AMOUNT 20 // Maxima cantidad de direcciones ip almacenadas
#define MAX_SSID_LENGTH 32 // Maxima longitud de la ssid, con delimitador '\0'
#define MAX_PASSWD_LENGTH 64 // Maxima longitud de contraseña
#define MIN_PASSWD_LENGTH 8
#define SENSOR_INFO_LENGTH 1 // Longitud de datos de pines del maestro
#define GROUP_INFO_LENGTH (GROUP_NAME_LENGTH+MOTION_DET_TIME_LENGTH+1) // Longitud de informacion de grupos, nombre, tiempo de pir y excepcion
#define GROUP_AMOUNT 10 // Cantidad de grupos
#define NAME_LENGTH 10
#define PIR_TIME_LENGTH 4
#define EXT_IP_LENGTH 4

// Direcciones de memoria del maestro
#define SSID_DIR 0
#define PASSWD_DIR (SSID_DIR + MAX_SSID_LENGTH)
#define SLAVE_IP_DIR (PASSWD_DIR + MAX_PASSWD_LENGTH)
#define SENSOR_INFO_DIR (SLAVE_IP_DIR + MAX_IP_AMOUNT)
#define PIR_TIME_DIR (SENSOR_INFO_DIR + SENSOR_INFO_LENGTH) // Tiempo de inactividad del pir
#define NAME_DIR (PIR_TIME_DIR + PIR_TIME_LENGTH)
#define EXT_SSID_DIR (NAME_DIR + NAME_LENGTH)
#define EXT_PASSWD_DIR (EXT_SSID_DIR + MAX_SSID_LENGTH)
#define EXT_IP_DIR (EXT_PASSWD_DIR + MAX_PASSWD_LENGTH)
#define RELAY_MODE_DIR (EXT_IP_DIR + EXT_IP_LENGTH)
#define GROUP_INFO_DIR (RELAY_MODE_DIR + MAX_MASTER_RELAY_AMOUNT)
#define ALARMS_DIR (GROUP_INFO_DIR + GROUP_INFO_LENGTH*GROUP_AMOUNT) // Tambien usa '\0' como delimitador
#define DELIMITER 0

// Direcciones de memoria de esclavo
#define IP_DIR (PASSWD_DIR + MAX_PASSWD_LENGTH)
#define SENSOR_DIR (IP_DIR+4)
#define RELAY_GROUP_DIR (SENSOR_DIR + 1)
#define IR_CONTROL_GROUP_DIR (RELAY_GROUP_DIR +  MAX_SLAVE_RELAY_AMOUNT)

// Posicion de sensores en maestro y esclavo (a partir de direccion de sensores)
#define PIR_DIR 0
#define TEMP_DIR 1
#define IR_DIR 2
#define RELAY_DIR 3

// Configuración por defecto para Wifi
#define DEFAULT_SSID "node1"
#define DEFAULT_PASSWD "12345678" // Tiene que tener por lo menos 8 caracteres
#define DEFAULT_PORT 4000
#define DEFAULT_EXT_PORT 8081

// Configuracion por defecto para pir
#define DEFAULT_MOTION_DET_TIME 20

// Estructura para manejar la informacion de un relay, tanto en esclavo como maestro
typedef struct r{
  r(): connected(false), on(false), group(DISCONNECTED){};
  bool on; // Indica si el relay esta encendido
  bool connected; // Indica si el relay esta en uso
  int group; // Indica a que grupo pertenece un relay
  int pin; // Indica en que pin se encuentra conectado un relay
} relayInfo;

#endif