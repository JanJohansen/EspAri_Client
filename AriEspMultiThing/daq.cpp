#include "daq.h"

DAQ::DAQ(uint16 averageCount, uint8 precision, float hysteresis)
{
  _averageCount = averageCount;
  _precision = precision;
  _hysteresis = hysteresis;

  _dataCount = 0;
  _averagingSum = 0;
  _valueHandler = 0;
  _lastValue = 99999999;
}

void DAQ::handleValue(float value){
  // Handle averaging if used...
  _averagingSum += value;
  _dataCount++;
  
  if(_dataCount == _averageCount){
    if(_valueHandler){
      float value = _averagingSum / _dataCount;

      // Don't send same val...! :O(
      // TODO - Use hysteresis.
      //if(value != _lastValue){
      if((value < (_lastValue - _hysteresis)) || (value > (_lastValue + _hysteresis))){
        char buf[20];
        dtostrf(value, 0, _precision, buf);
        _valueHandler(buf);
        _lastValue = atof(buf);
      }
    }
    _dataCount = 0; 
    _averagingSum = 0;
  }
}

void DAQ::setValueHandler(void (*valueHandlerFunction)(char *pValueString)){
  _valueHandler = valueHandlerFunction;
}

