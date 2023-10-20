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
	EEPROM.commit();
	Serial.printf("\n EEPROM clean.");
}

void loop(){

}
