#ifndef daq_h
#define daq_h

#include <Arduino.h>

class DAQ
{
  public:
    DAQ(uint16 averageCount, uint8 precision, float hysteresis);
    void handleValue(float value);
    void setValueHandler(void (*valueHandlerFunction)(char *pValueString));
  private:
    float _lastValue;
    uint8 _precision;
    float _hysteresis;
    uint16 _averageCount;
    uint16 _dataCount;
    float _averagingSum;

    void (*_valueHandler)(char *pValueString) ;
};

#endif

