#include <EEPROM.h>
#include <SPU.h>
void setup(){
	Serial.begin(115200);
	EEPROM.begin(1024);
	for(int i = 0; i < 1024; i++){
		EEPROM.write(i, 0);
	}
	EEPROM.commit();
	Serial.printf("\n EEPROM clean.");
}

void loop(){

}
