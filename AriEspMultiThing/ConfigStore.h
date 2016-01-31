#ifndef configstore_h
#define configstore_h

#include <Arduino.h>
#include <EEPROM.h>
#include "ConfigData.h"

class ConfigStore
{
  public:
    
    static ConfigData* load();
    static byte isDataValid(ConfigData* pConfigData);
    static void free();
    static void save(ConfigData* pConfigData);
  
  private:
    ConfigStore();
};

#endif

