#include <EEPROM.h>
#include <SPU.h>
void setup(){
	Serial.begin(115200);
	EEPROM.begin(1024);

  for(int i = 0; i < 1024; i++){
		EEPROM.write(i, 0);
	}
	for(int i = 0; i < MAX_MASTER_RELAY_AMOUNT; i++){
		EEPROM.write(i+RELAY_MODE_DIR, 255);
	}
  Serial.printf("\n EEPROM clean.");

  String ssid= "Proyecto_Energia_AP";
  String passwd= "PrOyEcTo2023!";
  for (int i = 0; i < ssid.length(); i++) {
    EEPROM.write(i + SSID_DIR, ssid[i]);
  }
  for (int i = 0; i < passwd.length(); i++) {
    EEPROM.write(i + PASSWD_DIR, passwd[i]);
  }

  // Set relay group for each relay 
  EEPROM.write(RELAY_MODE_DIR , 0);
  EEPROM.write(RELAY_MODE_DIR + 1, 0);

  // Define value of 8 bits for sensors info (63 is IR and relays actives)
  EEPROM.write(SENSOR_DIR, 63);
	
  EEPROM.commit();
	Serial.printf("\n EEPROM set.");
}

void loop(){

}
