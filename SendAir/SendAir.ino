/* IRremoteESP8266: IRsendDemo - demonstrates sending IR codes with IRsend.
 *
 * Version 1.1 January, 2019
 * Based on Ken Shirriff's IrsendDemo Version 0.1 July, 2009,
 * Copyright 2009 Ken Shirriff, http://arcfn.com
 *
 * An IR LED circuit *MUST* be connected to the ESP8266 on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/markszabo/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */

#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#include <IRremoteESP8266.h>
#include <IRsend.h>

const uint16_t kIrLed = 15;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
const int buttonPin = 13;     // the number of the pushbutton pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
int airState=0;

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

uint16_t rawOn[200] = {4442, 4348, 634, 1504, 582, 490, 656, 1482, 656, 1530, 582, 464, 632, 464, 656, 1504, 582, 512, 660, 408
, 634, 1508, 604, 488, 634, 440, 654, 1506, 582, 1582, 634, 414, 630, 1534, 604, 466, 656, 438, 656, 414
, 656, 1508, 604, 1534, 630, 1556, 582, 1584, 608, 1530, 604, 1560, 634, 1484, 626, 1536, 656, 412, 630, 466
, 654, 414, 656, 436, 658, 412, 656, 440, 656, 1502, 636, 1508, 602, 1534, 604, 1582, 634, 414, 628, 466
, 630, 438, 654, 1510, 602, 488, 608, 464, 656, 414, 630, 466, 654, 1482, 656, 1508, 602, 1536, 602, 5156
, 4440, 4302, 656, 1504, 582, 492, 654, 1504, 610, 1556, 582, 466, 656, 460, 660, 1456, 602, 514, 634, 434
, 636, 1530, 582, 466, 654, 440, 654, 1484, 602, 1584, 634, 412, 656, 1530, 582, 488, 636, 438, 654, 414
, 656, 1510, 654, 1530, 582, 1536, 628, 1534, 602, 1556, 582, 1562, 656, 1508, 602, 1538, 628, 464, 656, 414
, 656, 432, 636, 440, 654, 436, 634, 440, 656, 1504, 608, 1534, 602, 1562, 654, 1484, 602, 490, 632, 438
, 656, 414, 680, 1486, 600, 492, 654, 414, 654, 414, 656, 460, 610, 1508, 680, 1504, 632, 1532, 634};


// Example of data captured by IRrecvDumpV2.ino
/*uint16_t rawOn[219] = {4896, 206, 1924, 214, 872, 214, 1922, 216, 1948, 212, 856, 218, 876, 222, 1916, 218, 316, 66, 494, 172
, 336, 66, 494, 158, 1446, 68, 484, 172, 344, 68, 474, 190, 362, 68, 472, 198, 1398, 70, 470, 198
, 1424, 76, 462, 216, 314, 70, 466, 236, 1390, 70, 468, 228, 308, 114, 416, 196, 380, 162, 354, 168
, 1452, 210, 306, 168, 1496, 134, 364, 174, 1454, 206, 330, 112, 1488, 236, 302, 100, 1528, 212, 324, 96
, 1504, 206, 330, 96, 1532, 206, 330, 96, 1504, 246, 290, 98, 458, 242, 294, 100, 432, 220, 316, 94
, 464, 220, 316, 98, 434, 224, 312
, 102, 454, 224, 314, 114, 416, 232, 304, 114, 1512, 236, 300, 162
, 368, 224, 312, 122, 436, 218, 320, 128, 1480, 226, 302, 154, 404, 234, 302, 106, 426, 268, 268, 104
, 454, 254, 282, 102, 430, 230, 306, 100, 432, 208, 328, 106, 1522, 240, 296, 114, 1514, 256, 278, 222
, 314, 278, 254, 174, 1458, 306, 226, 152, 1454, 390, 116, 148, 1510, 654, 1490, 624, 5118, 4470, 4298, 622
, 1516, 648, 448, 646, 1492, 598, 1566, 580, 490, 608, 488, 618, 1518, 588, 512, 606, 472, 610, 1546, 620
, 458, 586, 510, 616, 1514, 564, 1604, 558, 514, 570, 1582, 602, 478, 634, 460, 616, 1534, 632, 1504};*/


/*uint16_t rawOff[200] = {4440, 4348, 582, 1534, 602, 492, 632, 1508, 602, 1584, 658, 386, 658, 438, 684, 1456, 656, 438, 656, 412
, 656, 1530, 580, 466, 656, 460, 658, 1480, 582, 1582, 608, 438, 658, 1506, 686, 404, 636, 1530, 632, 1506
, 632, 1510, 602, 1584, 634, 412, 656, 1532, 580, 1558, 608, 1556, 582, 466, 656, 438, 654, 414, 656, 438
, 656, 1482, 656, 440, 630, 438, 630, 1556, 610, 1530, 582, 1562, 652, 416, 654, 438, 630, 436, 654, 464
, 636, 412, 656, 438, 680, 388, 656, 414, 656, 1508, 656, 1530, 632, 1484, 658, 1508, 602, 1558, 608, 5108
, 4486, 4298, 580, 1558, 634, 460, 632, 1506, 580, 1584, 660, 408, 634, 440, 656, 1504, 636, 438, 658, 412
, 676, 1488, 654, 434, 634, 438, 656, 1482, 656, 1510, 602, 488, 660, 1482, 656, 414, 708, 1458, 602, 1562
, 656, 1504, 580, 1562, 654, 436, 634, 1532, 580, 1558, 634, 1530, 580, 514, 608, 436, 656, 414, 654, 460
, 634, 1484, 654, 440, 630, 460, 634, 1508, 656, 1508, 602, 1558, 634, 440, 654, 414, 656, 414, 656, 460
, 634, 412, 656, 438, 630, 460, 636, 438, 656, 1482, 602, 1584, 634, 1504, 636, 1530, 580, 1564, 654};*/

uint16_t rawOff[99] = {//15580, 
  4350, 4300, 550, 1600, 550, 500, 550, 1600, 500, 1650, 
  500, 550, 550, 500, 550, 1600, 500, 550, 550, 500, 
  550, 1600, 550, 500, 550, 550, 550, 1550, 550, 1600, 
  550, 500, 550, 1600, 550, 500, 550, 1600, 550, 1550, 
  550, 1600, 550, 1600, 500, 550, 550, 1600, 500, 1600, 
  550, 1600, 550, 500, 550, 550, 550, 500, 550, 550, 
  550, 1550, 550, 550, 550, 500, 550, 1600, 500, 1600, 
  550, 1600, 500, 550, 550, 550, 550, 500, 550, 550, 
  500, 550, 550, 500, 550, 550, 500, 550, 550, 1600, 
  500, 1600, 550, 1600, 500, 1650, 500, 1600, 500};

double t = 0;

void setup() {
  irsend.begin();
  pinMode(buttonPin, INPUT);   // initialize the pushbutton pin as an input:
  Serial.begin(115200);//, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.printf("\n Start");
}

void loop() {
  /*buttonState = digitalRead(buttonPin); // read the state of the pushbutton value:
   // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if ((buttonState == LOW) && ((millis() - t) > 500 )) {
    t = millis();
    if (airState == LOW){
      Serial.println("Enviando Encendido");
      irsend.sendRaw(rawOn, 200, 38);  // Send a raw data capture at 38kHz.
      airState = HIGH;
    }
    else{
      Serial.println("Enviando Apagado");
      irsend.sendRaw(rawOff, 200, 38);  // Send a raw data capture at 38kHz.
      airState = LOW;
    }
  }*/
  Serial.println("Enviando codigo.");
  irsend.sendRaw(rawOff, 99, 38);
  delay(2000);
}
