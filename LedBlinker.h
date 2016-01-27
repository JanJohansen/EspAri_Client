#ifndef ledblinker_h
#define ledblinker_h

#include <Arduino.h>

class LedBlinker
{
  public:
    LedBlinker(byte ledPin);
    void setIntervals(uint16 onTime, uint16 offTime);
    void loop();
  private:
    byte ledPin;
    unsigned long previousMillis = 0;
    uint16 ledIntervals[2] = {100, 100};
    uint8 ledIntervalPos = 0;
};

#endif

