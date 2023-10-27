#include <EEPROM.h>
#include <SPU.h>
void setup(){
	Serial.begin(115200);
	EEPROM.begin(1024);

  // Set relay group for each relay 
  EEPROM.write(RELAY_MODE_DIR , 0);
  EEPROM.write(RELAY_MODE_DIR + 1, 0);

  String ssid= "Proyecto_Energia_AP";
  String passwd= "PrOyEcTo2023!";
  for (int i = 0; i < ssid.length(); i++) {
    EEPROM.write(i + SSID_DIR, ssid[i]);
  }
  for (int i = 0; i < passwd.length(); i++) {
    EEPROM.write(i + PASSWD_DIR, passwd[i]);
  }

  // Define value of 8 bits for sensors info (95 is dht active and pir inactive)
  EEPROM.write(SENSOR_INFO_DIR, 95);
  
  // Initial IP slave
  String ip= "192.168.0.210";
  for (int i = 0; i < ip.length(); i++)
    EEPROM.write(i + SLAVE_IP_DIR, ip[i]);

  int pir_time = 5;
  EEPROM.write(PIR_TIME_DIR, pir_time);
  EEPROM.write(PIR_TIME_DIR + 1, pir_time);
  EEPROM.write(PIR_TIME_DIR + 2, pir_time);
  EEPROM.write(PIR_TIME_DIR + 3, pir_time);  

	EEPROM.commit();
	Serial.printf("\n EEPROM set.");
}

void loop(){

}
