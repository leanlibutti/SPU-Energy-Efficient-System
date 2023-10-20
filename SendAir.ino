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

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
const int buttonPin = 2;     // the number of the pushbutton pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
int airState=0;

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// Example of data captured by IRrecvDumpV2.ino
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

void setup() {
  irsend.begin();
  pinMode(buttonPin, INPUT);   // initialize the pushbutton pin as an input:
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
}

void loop() {
  buttonState = digitalRead(buttonPin); // read the state of the pushbutton value:
   // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == LOW) {
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
  }
}
