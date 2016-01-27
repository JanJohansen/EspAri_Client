#ifndef daq_h
#define daq_h

#include <Arduino.h>

class DAQ
{
  public:
    DAQ(uint16 averageCount, uint8 precision, bool preventFlicker);
    void handleValue(float value);
    void setValueHandler(void (*valueHandlerFunction)(char *pValueString));
  private:
    float _lastValue;
    float _lastUnique;
    uint16 _averageCount;
    uint16 _dataCount;
    float _averagingSum;
    uint8 _precision;
    bool _preventFlicker;
    void (*_valueHandler)(char *pValueString) ;
};

#endif

