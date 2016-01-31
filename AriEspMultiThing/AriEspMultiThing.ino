#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "LedBlinker.h"

#include "AriClient.h"

#include <DHT.h>
#include "daq.h"

/*
- Make DallasSensors node.
- Make PCB/Box for "LivingroomLampThing"... RHTM (Relay, Humidity, Temperature, Motion)
*/


#define MOTION_SENSOR_DIGITAL_PIN 4
#define DHTPIN 5
#define DHTTYPE DHT22
#define RELAY_PIN 14
#define RELAY_ON 0
#define RELAY_OFF 1
#define LED_PIN 16


AriClient* pAri;
LedBlinker led(LED_PIN);
DHT dht(DHTPIN, DHTTYPE);
DAQ dhtTempDaq(10,1);
DAQ dhtHumDaq(10,1);

bool lastMotionState = false;

// app-Timer values
unsigned long appTimerStart;
unsigned long appTimerDelay = 6000;
boolean       appTimerTimedOut = true;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  delay(10);

  pAri = new AriClient("TestDevice");
  pAri->setValueCallback(handleSetValue);
  
  dht.begin();

  // Enable timer for temperature and humidity conversion...
  appTimerDelay = 5 * 1000;  // 60SECONDS = 1 MINUTE!
  appTimerTimedOut = false; 
  appTimerStart = millis(); // Restart timeout.

  dhtTempDaq.setValueHandler(dhtTempValueHandler);
  dhtHumDaq.setValueHandler(dhtHumValueHandler);

  // Set up relay output.
  digitalWrite(RELAY_PIN, RELAY_OFF);
  pinMode(RELAY_PIN, OUTPUT); 
}

void loop() {
  pAri->loop();
  led.loop();

  // Time to read temp? - Check timer
  if ((!appTimerTimedOut) && ((millis() - appTimerStart) > appTimerDelay)) {
    // timed out
    //appTimerTimedOut = true; // Disable timer...
    appTimerStart = millis(); // Restart timeout.
    

    float h = dht.readHumidity();
    float t = dht.readTemperature();
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      pAri->sendString("ERR", "DHT read error.");
      return;
    }

    char buf[10];
    dtostrf(t, 0, 1,buf);
    Serial.print("T: ");
    Serial.print(t);
    pAri->sendNumber("rawTemp", buf);

    dtostrf(h, 0, 1,buf);
    Serial.print("\tH: ");
    Serial.println(h);
    pAri->sendNumber("rawHum", buf);

    dhtTempDaq.handleValue(t);
    dhtHumDaq.handleValue(h);
  } 

  // Any motion?
  bool motion = digitalRead(MOTION_SENSOR_DIGITAL_PIN);
  if(lastMotionState != motion)
  {
    lastMotionState = motion;
    Serial.print("Motion: ");
    Serial.println(motion ? "1" : "0");
    pAri->sendNumber("motion", motion ? "1" : "0");
  }

  // Blink according to state!
  if(pAri->state == AriClient::STATE_CONNECTED) led.setIntervals(50, 1950);
  else if(pAri->state == AriClient::STATE_WAIT4WIFI) led.setIntervals(50, 450);
  else if(pAri->state == AriClient::STATE_WAIT4ARI) led.setIntervals(50, 950);
  else led.setIntervals(50, 150);
}

void dhtTempValueHandler(char *pValueString){
  Serial.print("New Temperature: ");
  Serial.println(pValueString);
  pAri->sendNumber("temperature", pValueString);
}

void dhtHumValueHandler(char *pValueString){
  Serial.print("New Humidity: ");
  Serial.println(pValueString);
  pAri->sendNumber("humidity", pValueString);
}

void handleSetValue(const char* pName, const char* pValue){
  if(strcasecmp(pName, "light") == 0){
    Serial.print("LIGHT = ");
    Serial.println(pValue);

    if((*pValue == 1) || (*pValue == '1') || (*(pValue+1) == 'n') || (*(pValue+1) == 'N')) digitalWrite(RELAY_PIN, RELAY_ON);
    else digitalWrite(RELAY_PIN, RELAY_OFF);
    pAri->sendString("light", pValue);
  }
}

