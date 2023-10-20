#define _DISABLE_TLS_
//#define _DEBUG_
#define THINGER_SERVER "192.168.100.1"

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ThingerESP8266.h>

#define USERNAME "jdeantueno"
#define DEVICE_ID "TestNode"
#define DEVICE_CREDENTIAL "820NYvH8vy6v"

#define SSID "my_ap"
#define SSID_PASSWORD "my_password"

ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  thing.add_wifi(SSID, SSID_PASSWORD);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}