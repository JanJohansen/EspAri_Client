#include "ConfigStore.h"

ConfigStore::ConfigStore(){
}

ConfigData* ConfigStore::load(){
  EEPROM.begin(sizeof(ConfigData));
  return (ConfigData*) EEPROM.getDataPtr();
}

byte ConfigStore::isDataValid(ConfigData* pConfigData){
  if((pConfigData != 0) && (pConfigData->checkSum == 42)) return 1;
  else return 0;
}

void ConfigStore::free(){
  EEPROM.end();
}

void ConfigStore::save(ConfigData* pConfigData){
  EEPROM.write(0, *(uint8_t*)pConfigData);  // MUST call write, to indicate EEPROM is "dirty" or else commit will be skipped.!
  pConfigData->checkSum = 42;
  EEPROM.commit();
}

void ConfigStore::clear(ConfigData* pConfigData){
  uint16 addr = 0;
  for(addr = 0; addr < sizeof(ConfigData); addr++){
    EEPROM.write(0, addr);
  }
  //pConfigData->checkSum = 42;
  //EEPROM.commit();
}


//**********************************************************************************
// Usage:


//**********************************************************************************
// EEPROM routines
/*
void AriClient::checkEeprom(){
  uint16 cs = getEepromCs();
  if(cs != 0) {
    logln("!ERROR! - EEPROM corrupted.");
    clearEeprom();
    logln("EEPROM has been reinitialized.");
  }
}

uint16 AriClient::getEepromCs(){
  EEPROM.begin(EEPROM_SIZE);
  uint16 cs = 0;
  for(int i = 0; i < EEPROM_SIZE; i+=2) {
    cs ^= EEPROM.read(i) | EEPROM.read(i+1);
  }
  EEPROM.end();
  return cs;
}

char* AriClient::readEepromString(uint16 address, char* pBuf, uint16 maxLength){
  EEPROM.begin(EEPROM_SIZE);
  char c;
  uint16 pos = 0;
  do{
    c = EEPROM.read(address++);
    pBuf[pos++] = c;
  } while((c != 0) && (pos <= maxLength));
  EEPROM.end();
  return pBuf;
}

void AriClient::writeEepromString(uint16 address, char* pBuf, uint16 maxLength){
  EEPROM.begin(EEPROM_SIZE);
  uint16 pos = 0;
  do{
    EEPROM.write(address, pBuf[pos++]);
    address++;
  }while((pBuf[pos] != 0) && (pos <= maxLength));
  EEPROM.end();
}

void AriClient::clearEeprom(){
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  uint16 cs = getEepromCs();
//  EEPROM.write(EEPROM_SOFTWARE_VERSION, SOFTWARE_VERSION);
//  EEPROM.write(EEPROM_CS, SOFTWARE_VERSION ^ 0xFFFF);
  EEPROM.end();
}
*/
