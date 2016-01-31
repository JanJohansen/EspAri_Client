#ifndef ari_client_h
#define ari_client_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "ConfigStore.h"

// For WifiManager
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>


class AriClient
{
  public:
    typedef void (*valueCBType)(const char* pName, const char* pValue);
    
    typedef enum { STATE_RESET, STATE_WAIT4WIFI, STATE_WAIT4ARI, STATE_WAIT4TOKEN, STATE_WAIT4AUTHREPLY, STATE_CONNECTED } States;
    States state;

    AriClient(const char* pDeviceName);
    void setValueCallback(valueCBType);
    void loop();
    void sendString(const char* pName, const char* pValue);
    void sendNumber(const char* pName, const char* pValue);
    
  private:
    typedef enum { EVENT_LOOP, EVENT_TIMEOUT, EVENT_MESSAGE, EVENT_TCP_CONNECTED, EVENT_TCP_DISCONNECTED, EVENT_WIFI_CONNECTED, EVENT_WIFI_DISCONNECTED } Events;

    const char* ssid     = "MiX2";
    const char* password = "jan190374";
    const uint16_t ariPort = 5000;
    const char* ariHost = "192.168.1.127"; // ip or url
    const char* pDeviceName = "AnonymousDevice";

    
    valueCBType pValueCB = 0;
    
    // Timer values
    unsigned long timerStart;
    unsigned long timerDelay = 5000;
    boolean       timerTimedOut = true;
    void setTimeout(unsigned long timeDelay);

    WiFiClient* pWifi;
    //WiFiClient client = *new WiFiClient();
    uint16 reqId = 0;
    
    // mem for reception of JSON in TCP telegrams.
    uint16 msgPos = 0;
    char msg[512];
    uint8 bracketCount = 0;
    
    bool wifiConnected = false;
    bool tcpConnected = false;
    
    ConfigData* pConfig;
    
    void handleEvent(uint8 event, void *pData);
    void handleWifiManager();
    void checkEeprom();
    uint16 getEepromCs();
    char* readEepromString(uint16 address, char* pBuf, uint16 maxLength);
    void writeEepromString(uint16 address, char* pBuf, uint16 maxLength);
    void clearEeprom();

};

#endif

