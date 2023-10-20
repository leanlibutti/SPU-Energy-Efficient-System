#include <Encoder.h>

Encoder encoder(D3, D4, D0);

void move(bool dir){
	if(dir) Serial.printf("\n move up");
	else Serial.printf("\n move down");
}

void setup(){
	Serial.begin(115200);
	encoder.onMove(move);
}

void loop(){
	encoder.check();
}