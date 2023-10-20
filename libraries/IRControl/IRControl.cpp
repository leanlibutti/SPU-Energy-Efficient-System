#include <IRControl.h>

uint16_t rawOff[200] = {4440, 4348, 582, 1534, 602, 492, 632, 1508, 602, 1584, 658, 386, 658, 438, 684, 1456, 656, 438, 656, 412
, 656, 1530, 580, 466, 656, 460, 658, 1480, 582, 1582, 608, 438, 658, 1506, 686, 404, 636, 1530, 632, 1506
, 632, 1510, 602, 1584, 634, 412, 656, 1532, 580, 1558, 608, 1556, 582, 466, 656, 438, 654, 414, 656, 438
, 656, 1482, 656, 440, 630, 438, 630, 1556, 610, 1530, 582, 1562, 652, 416, 654, 438, 630, 436, 654, 464
, 636, 412, 656, 438, 680, 388, 656, 414, 656, 1508, 656, 1530, 632, 1484, 658, 1508, 602, 1558, 608, 5108
, 4486, 4298, 580, 1558, 634, 460, 632, 1506, 580, 1584, 660, 408, 634, 440, 656, 1504, 636, 438, 658, 412
, 676, 1488, 654, 434, 634, 438, 656, 1482, 656, 1510, 602, 488, 660, 1482, 656, 414, 708, 1458, 602, 1562
, 656, 1504, 580, 1562, 654, 436, 634, 1532, 580, 1558, 634, 1530, 580, 514, 608, 436, 656, 414, 654, 460
, 634, 1484, 654, 440, 630, 460, 634, 1508, 656, 1508, 602, 1558, 634, 440, 654, 414, 656, 414, 656, 460
, 634, 412, 656, 438, 630, 460, 636, 438, 656, 1482, 602, 1584, 634, 1504, 636, 1530, 580, 1564, 654};

/*uint16_t rawOff[99] = {//15580, 
  4350, 4300, 550, 1600, 550, 500, 550, 1600, 500, 1650, 
  500, 550, 550, 500, 550, 1600, 500, 550, 550, 500, 
  550, 1600, 550, 500, 550, 550, 550, 1550, 550, 1600, 
  550, 500, 550, 1600, 550, 500, 550, 1600, 550, 1550, 
  550, 1600, 550, 1600, 500, 550, 550, 1600, 500, 1600, 
  550, 1600, 550, 500, 550, 550, 550, 500, 550, 550, 
  550, 1550, 550, 550, 550, 500, 550, 1600, 500, 1600, 
  550, 1600, 500, 550, 550, 550, 550, 500, 550, 550, 
  500, 550, 550, 500, 550, 550, 500, 550, 550, 1600, 
  500, 1600, 550, 1600, 500, 1650, 500, 1600, 500};*/

IRControl::IRControl(int p): irsend(IRsend(p)){
  outputPin = p;
  irsend.begin();
  pinMode(outputPin, OUTPUT);
  EEPROM.begin(MEMORY_SIZE);
}

void IRControl::begin(){
  codeLength = 0;
  byte code[2];
  Serial.printf("\n Codigo almacenado : [");
  int p = 0;
  for(int i = 0; i < IR_CODE_LENGTH; i++){
    code[0] = EEPROM.read(p+IR_CODE_DIR);
    p++;
    if(code[0] != '\0'){
      code[1] = EEPROM.read(p+IR_CODE_DIR);
      p++;
      memcpy(&rawCode[i], &code, 2);
      if(i % 10 == 0)
        Serial.printf("\n %d,", rawCode[i]);
      else
        Serial.printf(" %d,", rawCode[i]);
      codeLength++;
    } else break;
  }
  Serial.printf("]");
}

void IRControl::sendCode(){
  Serial.println("Enviando codigo");
  //irsend.sendRaw(rawCode, codeLength, 38);
  irsend.sendRaw(rawOff, 200, 38);
}

void IRControl::storeCode(unsigned short v[], int size){
  int i, p = 0;
  Serial.printf("\n Codigo recibido: [");
  for(i = 0; i < size; i++){
    byte code[2];
    memcpy(&code, &v[i], 2);
    EEPROM.write(p+IR_CODE_DIR, code[0]);
    p++;
    EEPROM.write(p+IR_CODE_DIR, code[1]);
    p++;
    rawCode[i] = v[i];
    if(i % 10 == 0)
      Serial.printf("\n %d,", rawCode[i]);
    else
      Serial.printf(" %d,", rawCode[i]);
  }
  Serial.printf("]");
  Serial.printf("\n Length %d", size);
  if(p+IR_CODE_DIR < MEMORY_SIZE)
    EEPROM.write(p+IR_CODE_DIR, '\0');
  EEPROM.commit();
}