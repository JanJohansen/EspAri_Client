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

#include "AriClient.h"

#define log(x) Serial.print(x)
#define logln(x) Serial.println(x)


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

      pConfig = ConfigStore::load();
      if(!ConfigStore::isDataValid(pConfig)){
        ConfigStore::free();
        logln("EEPROM DATA NOT OK - Starting WifiManager to get proper data!");
        handleWifiManager();    // Will reset and not return!
        return;
      }
      
      logln();
      log("Connecting to WiFi...");
      //logln(pConfig->ssid);

      WiFi.begin();
      //WiFi.begin((const char*)pConfig->ssid, (const char*)pConfig->wifiPassword); // NEED (const char*) cast!!!!???
      //WiFi.begin("MiX2", "jan190374");
      WiFi.mode(WIFI_STA);  // To stop announcing access point ssid.! (This is a bug workaround.)
      setTimeout(10000); // Wait 15 sec for reply.
      state = STATE_WAIT4WIFI;

      ConfigStore::free();
      
    break;
    //---------------------------------------------------------------------------
    case STATE_WAIT4WIFI:
      if(event == EVENT_WIFI_CONNECTED) {
        log("WiFi connected - IP address: ");
        logln(WiFi.localIP());
        
        // Connect to ARI server.
        // Get server IP from EEPROM config or...
        // TODO: Implement mDNS resolver for ARI server.
        pConfig = ConfigStore::load();
        log("connecting to ");
        logln(pConfig->ariServer);
        pWifi->connect(pConfig->ariServer, 5000); 
        setTimeout(5000); // Wait 5 sec for connection.
        state = STATE_WAIT4ARI;

        ConfigStore::free();
        
      } else if(event == EVENT_TIMEOUT){
          logln("Connecting to wifi failed, going into WifiManager mode.");
          setTimeout(0);  // Disable timeout.
          handleWifiManager();    // Will reset and not return!
      }
    break;
    //---------------------------------------------------------------------------
    case STATE_WAIT4ARI:
      if(event == EVENT_TCP_CONNECTED) {
        logln("Connected to ARI.");
        
        // TODO: check validity of EEPROM.
        pConfig = ConfigStore::load();
        if(!ConfigStore::isDataValid(pConfig)){
          ConfigStore::free();
          logln("EEPROM DATA NOT OK - Starting WifiManager to get proper data!");
          handleWifiManager();    // Will reset and not return!
          return;
        }
    
        // TODO: Read token form EEPROM.
        //char pBuf[256];
        //const char* pAuthToken = readEepromString(EEPROM_ARI_SERVER_AUTH_TOKEN_START, pBuf, 256);
        //const char* pAuthToken = "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJuYW1lIjoiRXNwVGVzdFRoaW5nIiwicm9sZSI6ImRldmljZSIsImNyZWF0ZWQiOiIyMDE2LTAxLTA1VDIyOjUyOjAzLjk1MFoifQ.hbHlCU8VuptO9wQMN8x1oC5HGN_-4ooer4JI4HaGPdA";
        char* pAuthToken = pConfig->authToken;
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
        
        ConfigStore::free();
        
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
        logln("Storing new authToken in EEPROM!");
        
        pConfig = ConfigStore::load();
        if(!ConfigStore::isDataValid(pConfig)){
          ConfigStore::free();
          logln("EEPROM DATA NOT OK - Starting WifiManager to get proper data!");
          handleWifiManager();    // Will reset and not return!
          return;
        }

        strncpy(pConfig->authToken, pAuthToken, sizeof(pConfig->authToken));
        ConfigStore::save(pConfig);
  
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

        ConfigStore::free();
  
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

        // TODO: Handle if not authenticated!
  
        String tlg = "{\"cmd\":\"SETCLIENTINFO\",\"pars\":{\"name\":\"";
        tlg += pDeviceName;
        tlg += "\",\"values\":{";
        tlg += "\"temperature\":{},";
        tlg += "\"humidity\":{},";
        tlg += "\"motion\":{},";
        tlg += "\"light\":{},";
        tlg += "\"pwm\":{}";
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

/***************************************************************************************/
byte shouldSaveConfig = 0;

void configModeCallback (WiFiManager *myWiFiManager) {
  //if you used auto generated SSID, print it
  log("Entered config mode with SSID: ");
  logln(myWiFiManager->getConfigPortalSSID());
  log("To configgure device, please go to: ");
  logln(WiFi.softAPIP());

}

void saveConfigCallback () {
  logln("***** Should save config...");
  shouldSaveConfig = 1;
}

void printConfig(){
  ConfigData* pConfig = ConfigStore::load();

  logln("Configuration in EEPROM: ");
  //log("SSID: ");
  //logln(pConfig->ssid);
  //log("PW: ");
  //logln(pConfig->wifiPassword);
  log("DeviceName: ");
  logln(pConfig->deviceName);
  log("ariUrl: ");
  logln(pConfig->ariServer);
  log("authToken: ");
  logln(pConfig->authToken);
  log("checkSum: ");
  logln(pConfig->checkSum);

  ConfigStore::free();
}

// Handling of WifiManager mode... Reset/reboot when done.
void AriClient::handleWifiManager(){

  printConfig();


  // Custom parameters.
  char ariServer[50] = "192.168.1.127";
  char deviceName[50] = "EspAriDevice";

  // Overwrite with EEPROM stored data if available.
  ConfigData* pConfig = ConfigStore::load();
  if(!ConfigStore::isDataValid(pConfig)){
    strncpy(ariServer, pConfig->ariServer, sizeof(pConfig->ariServer));
    strncpy(deviceName, pConfig->deviceName, sizeof(pConfig->deviceName));
  }
  ConfigStore::free();

  WiFiManagerParameter wifiParam_ariServer("ARI_server", "IP or blank to use (mDNS) AutoDetect.", ariServer, sizeof(ariServer)-1);
  WiFiManagerParameter wifiParam_deviceName("ARI_device_name", "Name of this device.", deviceName, sizeof(deviceName)-1);

  // Config 
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setTimeout(300);
  wifiManager.addParameter(&wifiParam_ariServer);
  wifiManager.addParameter(&wifiParam_deviceName);
   
  if(!wifiManager.startConfigPortal("Ari_Device")) {
    logln("!!! Timeout...");
    // Reset and try again.
    //ESP.reset();
    //delay(1000);
  }

  //logln("WIFI Connected.");

  const char* pAriServer = wifiParam_ariServer.getValue();
  const char* pDeviceName = wifiParam_deviceName.getValue();

  logln("Config from WM.");
  log("SSID: ");
  logln(wifiManager.getConfigPortalSSID().c_str());
  //log("PW: ");
  //logln(wifiManager.getPassword());
  log("Device name: ");
  logln((char*)pDeviceName);
  log("Connecting to ARI server on: ");
  logln((char*)pAriServer);

  printConfig();

  //save the custom parameters to FS
  pConfig = ConfigStore::load();
  if( (!ConfigStore::isDataValid(pConfig)) || 
      (shouldSaveConfig) ||
      //(strcmp(pConfig->ssid, wifiManager.getConfigPortalSSID().c_str()) != 0) ||
      //(strcmp(pConfig->wifiPassword, wifiManager.getPassword().c_str()) != 0) ||
      (strcmp(pConfig->deviceName, pDeviceName) != 0) ||
      (strcmp(pConfig->ariServer, pAriServer) != 0)
  ){
    
    logln("Saving configuration...");

    if(strcmp(pConfig->deviceName, pDeviceName) != 0) pConfig->authToken[0] = 0; // Clear authToken if device has a new name.!

    //strncpy(pConfig->ssid, wifiManager.getConfigPortalSSID().c_str(), sizeof(pConfig->ssid));
    //strncpy(pConfig->wifiPassword, wifiManager.getPassword().c_str(), sizeof(pConfig->wifiPassword));
    strncpy(pConfig->deviceName, pDeviceName, sizeof(pConfig->deviceName));
    strncpy(pConfig->ariServer, pAriServer, sizeof(pConfig->ariServer));
    
    ConfigStore::save(pConfig);
  }
  ConfigStore::free();

  printConfig();

  ESP.reset();
  delay(1000);
}




