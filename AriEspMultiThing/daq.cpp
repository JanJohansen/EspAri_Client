#include "daq.h"

DAQ::DAQ(uint16 averageCount, uint8 precision)
{
  _averageCount = averageCount;
  _precision = precision;

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
      if(value != _lastValue){
        char buf[20];
        dtostrf(value, 0, _precision, buf);
        _valueHandler(buf);
        _lastValue = value;
      }
    }
    _dataCount = 0; 
    _averagingSum = 0;
  }
}

void DAQ::setValueHandler(void (*valueHandlerFunction)(char *pValueString)){
  _valueHandler = valueHandlerFunction;
}


/*  lv  llv   out
 25 0   0     25    25>0>0
 20 25  0     20    20<25
 19 20  25    19
 19 19  20    
 19     
 20
 19 20  19
 20 19  20
 21 20  19    21*/
 
/*void handleVal(value){
  if(lastVal != value){
    lastVal = value;
    if(lastLastVal != lastVal){
      lastLastVal = lastVal;
      if(lastLastVal > lastVal && lastVal > value) DOSTUFF!
      if(lastLastVal < lastVal && lastVal < value) DOSTUFF!
    }
  }
}*/

