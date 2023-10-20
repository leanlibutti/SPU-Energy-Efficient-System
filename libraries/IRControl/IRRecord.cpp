#include <IRRecord.h>

void dump(decode_results *results) {
  // Dumps out the decode_results structure.
  // Call this after IRrecv::decode()
  uint16_t count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  } else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  } else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  } else if (results->decode_type == RC5X) {
    Serial.print("Decoded RC5X: ");
  } else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  } else if (results->decode_type == RCMM) {
    Serial.print("Decoded RCMM: ");
  } else if (results->decode_type == PANASONIC) {
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->address, HEX);
    Serial.print(" Value: ");
  } else if (results->decode_type == LG) {
    Serial.print("Decoded LG: ");
  } else if (results->decode_type == JVC) {
    Serial.print("Decoded JVC: ");
  } else if (results->decode_type == AIWA_RC_T501) {
    Serial.print("Decoded AIWA RC T501: ");
  } else if (results->decode_type == WHYNTER) {
    Serial.print("Decoded Whynter: ");
  }
  serialPrintUint64(results->value, 16);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (uint16_t i = 1; i < count; i++) {
    if (i % 120 == 0)
      yield();  // Preemptive yield every 100th entry to feed the WDT.
    if (i & 1) {
      Serial.print(results->rawbuf[i] * RAWTICK, DEC);
    } else {
      Serial.print((uint32_t) results->rawbuf[i] * RAWTICK, DEC);
    }
    if (i%20 == 0)
      Serial.println();
    Serial.write(',');
    Serial.print(" ");
  }
  Serial.println();
}

IRRecord::IRRecord(int pin): irrecv(IRrecv(pin, IR_CODE_LENGTH)){
  pinMode(pin, INPUT);
  inputPin = pin;
  codeLength = 0;
}

bool IRRecord::record(){
  irrecv.enableIRIn();
  int trys = 0; bool failed;
  int time = millis();
  while((failed = !irrecv.decode(&results)) && ((millis()-time) < 20000 )){
    yield();
  }
  Serial.printf("\n Time: %d\n", millis() - time);
  if (!failed) {
    serialPrintUint64(results.value, 16);
    dump(&results);
    for(int i = 0; i < results.rawlen; i++){
      values[i]= results.rawbuf[i]*RAWTICK;
    }
    codeLength = results.rawlen;
  } else
    codeLength = 0;
  irrecv.disableIRIn();
  return !failed;
}

int IRRecord::getValues(unsigned short v[]){
  /*uint16_t rawOff[200] = {4440, 4348, 582, 1534, 602, 492, 632, 1508, 602, 1584, 658, 386, 658, 438, 684, 1456, 656, 438, 656, 412
, 656, 1530, 580, 466, 656, 460, 658, 1480, 582, 1582, 608, 438, 658, 1506, 686, 404, 636, 1530, 632, 1506
, 632, 1510, 602, 1584, 634, 412, 656, 1532, 580, 1558, 608, 1556, 582, 466, 656, 438, 654, 414, 656, 438
, 656, 1482, 656, 440, 630, 438, 630, 1556, 610, 1530, 582, 1562, 652, 416, 654, 438, 630, 436, 654, 464
, 636, 412, 656, 438, 680, 388, 656, 414, 656, 1508, 656, 1530, 632, 1484, 658, 1508, 602, 1558, 608, 5108
, 4486, 4298, 580, 1558, 634, 460, 632, 1506, 580, 1584, 660, 408, 634, 440, 656, 1504, 636, 438, 658, 412
, 676, 1488, 654, 434, 634, 438, 656, 1482, 656, 1510, 602, 488, 660, 1482, 656, 414, 708, 1458, 602, 1562
, 656, 1504, 580, 1562, 654, 436, 634, 1532, 580, 1558, 634, 1530, 580, 514, 608, 436, 656, 414, 654, 460
, 634, 1484, 654, 440, 630, 460, 634, 1508, 656, 1508, 602, 1558, 634, 440, 654, 414, 656, 414, 656, 460
, 634, 412, 656, 438, 630, 460, 636, 438, 656, 1482, 602, 1584, 634, 1504, 636, 1530, 580, 1564, 654};
codeLength = 200;*/
  for(int i = 0; i < codeLength; i++){
    v[i] = values[i];
  }
  return codeLength;
}

bool IRRecord::codeRecorded(){
  return codeLength > 0;
}