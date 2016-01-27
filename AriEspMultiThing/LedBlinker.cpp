#include "LedBlinker.h"

LedBlinker::LedBlinker(byte ledPin){
  this->ledPin = ledPin;
  pinMode(ledPin, OUTPUT);

}

void LedBlinker::loop(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= ledIntervals[ledIntervalPos]) {
    
    if(ledIntervalPos & 1) digitalWrite(ledPin, 0);
    else digitalWrite(ledPin, 1);
    
    previousMillis = currentMillis;
    ledIntervalPos++;
    if(ledIntervalPos >= (sizeof(ledIntervals) / sizeof(uint16))) ledIntervalPos = 0;
  }
}

void LedBlinker::setIntervals(uint16 onTime, uint16 offTime){
  ledIntervals[0] = onTime;
  ledIntervals[1] = offTime;
}

