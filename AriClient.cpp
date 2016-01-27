/*
TODOs
- DONE: JSON decoding of tlg's
  - DONE: handle value setting from server
  - DONE: Control relay(s).
- DONE: Implement update limiter using "hysteresis"...
- DONE: Implment blinking LED to indicate status.!

- Implement WifiManager
  - Store config in EEPROM.
- Class-ify AriClient
- Push all to GIT!

- Make DallasSensors node.
- Make PCB/Box for "LivingroomLampThing"... RHTM (Relay, Humidity, Temperature, Motion)
*/

#define log(x) Serial.print(x)
#define logln(x) Serial.println(x)

#include "AriClient.h"

// EEPROM definitions.
#define EEPROM_SIZE 512
#define EEPROM_SOFTWARE_VERSION 0
#define EEPROM_CS 2
#define EEPROM_WIFI_SSID_START 2+2
#define EEPROM_WIFI_PASSWORD_START 4+33
#define EEPROM_ARI_SERVER_URL_START 4+33+65
#define EEPROM_ARI_SERVER_PORT_START 4+33+65+128
#define EEPROM_ARI_SERVER_AUTH_TOKEN_START 4+33+65+128+2
#define EEPROM_END 4+33+65+128+2+257
/*
#define EEPROM_SIZE 512
#define EEPROM_SOFTWARE_VERSION 0                   // uint16 version. Must match hardcoded value in SW.
#define EEPROM_CS 2                                 // CS to make EEPROM sum = 0.
#define EEPROM_WIFI_SSID_START 2+2                  // Max SSID length is 32 + \0
#define EEPROM_WIFI_PASSWORD_START 4+33             // According to 802.11i, max PSK or passphrase is 64 characters + \0
#define EEPROM_ARI_SERVER_URL_START 4+33+65         // max URL length is CHOSEN to be 127 characters + \0
#define EEPROM_ARI_SERVER_PORT_START 4+33+65+128    // Port is a 16bit integer = 2 bytes.
#define EEPROM_ARI_SERVER_AUTH_TOKEN_START 4+33+65+128+2  // Authentification token is CHOSEN to be limited to 256 characters
#define EEPROM_END 4+33+65+128+2+257                // Authentification token is CHOSEN to be limited to 256 characters (typical = 180-200)
*/



//AriClient::AriClient(const char* pSSID, const char* pPassword, const char* pDeviceName){
AriClient::AriClient(const char* pDeviceName){
  //Serial.begin(115200);
  //delay(10);
  logln("Creating ARI client.");
  this->pDeviceName = pDeviceName;
  pWifi = new WiFiClient();
  state = STATE_RESET;
  handleEvent(EVENT_LOOP, 0);
  
}

void AriClient::setValueCallback(valueCBType cbFunc){
  pValueCB = cbFunc;
}

void AriClient::loop(){
  // Check if data has been received, and form JSON message...
  while(pWifi->available()){
    char c = pWifi->read();
    if(msgPos < sizeof(msg)) msg[msgPos++] = c;
    if(c =='{') bracketCount++;
    else if(c == '}') {
      bracketCount--;
      if(bracketCount == 0){
        msg[msgPos] = 0;  // Zero terminate
        log("--> ");
        logln(msg);
        handleEvent(EVENT_MESSAGE, &msg);
        msgPos = 0;
      }
    }
  }

  // Check timer
  if ((!timerTimedOut) && ((millis() - timerStart) > timerDelay)) {
    // timed out
    timerTimedOut = true; // don't do this again
    
    // Fire event.
    handleEvent(EVENT_TIMEOUT, 0);
  }

  // Check Wifi connection.
  if((WiFi.status() == WL_CONNECTED) != wifiConnected){
    wifiConnected ^= 1;
    if(wifiConnected) {
      logln("Wifi connected.");
      handleEvent(EVENT_WIFI_CONNECTED, 0);
    }
    else {
      logln("Wifi disconnected!");
      tcpConnected = 0;
      handleEvent(EVENT_WIFI_DISCONNECTED, 0);
    }
  } else if(pWifi->connected() != tcpConnected){     // Check TCP conection.
    tcpConnected ^= 1;
    if(tcpConnected) {
      logln("TCP connected.");
      handleEvent(EVENT_TCP_CONNECTED, 0);
    }
    else {
      logln("TCP discconnected.");
      handleEvent(EVENT_TCP_DISCONNECTED, 0);
    }
  }
}

void AriClient::setTimeout(unsigned long timeDelay) {
  if(timeDelay == 0) timerTimedOut = true;  // Stop timer.
  else {
    timerStart = millis();
    timerDelay = timeDelay;
    timerTimedOut = false;
  }
}

//-----------------------------------------------------------------
void AriClient::handleEvent(uint8 event, void *pData){
  log("S:");
  log(state);
  log(" - E:");
  logln(event);
  
  switch(state){
    //---------------------------------------------------------------------------
    case STATE_RESET:
      // Connect to a WiFi network
      // TODO: Get SSID + password from EEPROM config.
      // TODO: If not connecting to wifi, start as AP and receive configurtaion.
      logln();
      log("Connecting to ");
      logln(ssid);
      
      WiFi.begin(ssid, password);
      state = STATE_WAIT4WIFI;
    break;
    //---------------------------------------------------------------------------
    case STATE_WAIT4WIFI:
      if(event == EVENT_WIFI_CONNECTED) {
        log("WiFi connected - IP address: ");
        logln(WiFi.localIP());
        
        // Connect to ARI server.
        // TODO: Get server IP from EEPROM config.
        log("connecting to ");
        logln(ariHost);
        pWifi->connect(ariHost, ariPort);
    
        setTimeout(5000); // Wait 5 sec for connection.
    
        state = STATE_WAIT4ARI;
      }
    break;
    //---------------------------------------------------------------------------
    case STATE_WAIT4ARI:
      if(event == EVENT_TCP_CONNECTED) {
        logln("Connected to ARI.");
        
        // TODO: check validity of EEPROM.
        //checkEeprom();
    
        // TODO: Read token form EEPROM.
        //char pBuf[256];
        //const char* pAuthToken = readEepromString(EEPROM_ARI_SERVER_AUTH_TOKEN_START, pBuf, 256);
        //const char* pAuthToken = "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJuYW1lIjoiRXNwVGVzdFRoaW5nIiwicm9sZSI6ImRldmljZSIsImNyZWF0ZWQiOiIyMDE2LTAxLTA1VDIyOjUyOjAzLjk1MFoifQ.hbHlCU8VuptO9wQMN8x1oC5HGN_-4ooer4JI4HaGPdA";
        const char* pAuthToken = "\0";
        if( *pAuthToken == 0) {
          // Request token.
          String msg = "{\"req\":\"";
          msg += reqId++;
          msg += "\",\"cmd\":\"REQAUTHTOKEN\",\"pars\":{\"name\":\"";
          msg += (char*)pDeviceName;
          msg += "\",\"role\":\"device\"}}";
          pWifi->print(msg);
          setTimeout(2000); // Wait 2 sec for reply.
          state = STATE_WAIT4TOKEN;
          
        } else {
          // Send auth token to server.
          String msg = "{\"req\":\"";
          msg += reqId++;
          msg += "\",\"cmd\":\"CONNECT\",\"pars\":{\"name\":\"";
          msg += (char*)pDeviceName;
          msg += "\",\"authToken\":\"";
          msg += (char*)pAuthToken;
          msg += "\"}}";
          pWifi->print(msg);
          setTimeout(2000); // Wait 2 sec for reply.
          state = STATE_WAIT4AUTHREPLY;

        }
      } else if(event == EVENT_TIMEOUT){
          log("connecting to ");
          logln(ariHost);
          pWifi->connect(ariHost, ariPort);
          setTimeout(5000); // Wait 5 sec for reply.
      } else if(event == EVENT_WIFI_DISCONNECTED) {
        state = STATE_RESET;
        handleEvent(EVENT_LOOP, 0);
      }
    break;
    //---------------------------------------------------------------------------
    case STATE_WAIT4TOKEN:
      if(event == EVENT_MESSAGE){
        log("Reply:");
        logln(msg);
  
        // Get authToken from JSON.
        StaticJsonBuffer<JSON_OBJECT_SIZE(10)> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject((char*)pData);
        if (!root.success()) {
          logln("Error parsing JSON! Ignoring...");
          return;
        }
        const char* pAuthToken = root["result"]["authToken"];
        log("authToken: ");
        logln(pAuthToken);
        
        // Store authToken in EEPROM.
  
  
        // Send auth token to server.
        String tlg = "{\"req\":\"";
        tlg += reqId++;
        tlg += "\",\"cmd\":\"CONNECT\",\"pars\":{\"name\":\"";
        tlg += (char*)pDeviceName;
        tlg += "\",\"authToken\":\"";
        tlg += (char*)pAuthToken;
        tlg += "\"}}";
        pWifi->print(tlg);
        setTimeout(2000); // Wait 2 sec for reply.
        state = STATE_WAIT4AUTHREPLY;
  
      } else if(event == EVENT_TCP_DISCONNECTED) {
        state = STATE_WAIT4WIFI;
        handleEvent(EVENT_WIFI_CONNECTED, 0);
      } else if(event == EVENT_WIFI_DISCONNECTED) {
        state = STATE_RESET;
        handleEvent(EVENT_LOOP, 0);
      }
    break;
    //---------------------------------------------------------------------------
    case STATE_WAIT4AUTHREPLY:
      if(event == EVENT_MESSAGE){
        log("Reply:");
        logln((char*)pData);
  
        String tlg = "{\"cmd\":\"SETCLIENTINFO\",\"pars\":{\"name\":\"";
        tlg += pDeviceName;
        tlg += "\",\"values\":{";
        tlg += "\"temperature\":{},";
        tlg += "\"humidity\":{},";
        tlg += "\"motion\":{},";
        tlg += "\"light\":{}";
        tlg += "}}}";
        pWifi->print(tlg);
        setTimeout(0); // No timeout since we don't await a reply.
        state = STATE_CONNECTED;

      } else if(event == EVENT_TCP_DISCONNECTED) {
        state = STATE_WAIT4WIFI;
        handleEvent(EVENT_WIFI_CONNECTED, 0);
      } else if(event == EVENT_WIFI_DISCONNECTED) {
        state = STATE_RESET;
        handleEvent(EVENT_LOOP, 0);
      }
    break;
    //---------------------------------------------------------------------------
    case STATE_CONNECTED:
      if(event == EVENT_MESSAGE){
          log("Msg:");
          log((char*)pData);

          StaticJsonBuffer<JSON_OBJECT_SIZE(10)> jsonBuffer;
          JsonObject& root = jsonBuffer.parseObject((char*)pData);
          if (!root.success()) {
            logln("Error parsing JSON! Ignoring...");
            return;
          }
          const char* pCmd = root["cmd"];
          //log("cmd: ");
          //logln(pCmd);
          if(strcmp(pCmd, "SETVALUE") == 0){
            const char* pName = root["pars"]["name"];  
            const char* pValue = root["pars"]["value"];
            if(pValueCB != 0) pValueCB(pName, pValue);
          }
          
          state = STATE_CONNECTED;
          
      } else if(event == EVENT_TCP_DISCONNECTED) {
        state = STATE_WAIT4WIFI;
        handleEvent(EVENT_WIFI_CONNECTED, 0);
      } else if(event == EVENT_WIFI_DISCONNECTED) {
        state = STATE_RESET;
        handleEvent(EVENT_LOOP, 0);
      }
    break;
  }
}

void AriClient::sendString(const char* pName, const char* pValue){
  String tlg = "{\"cmd\":\"VALUE\",\"pars\":{\"name\":\"";
  tlg += pName;
  tlg += "\",\"value\":\"";
  tlg += pValue;
  tlg += "\"}}";
  pWifi->print(tlg);
}

void AriClient::sendNumber(const char* pName, const char* pValue){
  String tlg = "{\"cmd\":\"VALUE\",\"pars\":{\"name\":\"";
  tlg += pName;
  tlg += "\",\"value\":";
  tlg += pValue;
  tlg += "}}";
  pWifi->print(tlg);
}



//**********************************************************************************
// EEPROM routines
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

