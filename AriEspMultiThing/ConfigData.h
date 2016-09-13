#include <Arduino.h>

typedef struct {
//  char swVersion[6];    // uint16 version. Must match hardcoded value in SW.
//  char ssid[33];        // Max SSID length is 32 + \0
//  char wifiPassword[65];// According to 802.11i, max PSK or passphrase is 64 characters + \0
  char ariServer[128];     // max URL length is CHOSEN to be 127 characters + \0
//  uint16 ariPort;       // Port is a 16bit integer = 2 bytes.
  char deviceName[50];  // Name of the device!
  char authToken[256];  // Authentification token is CHOSEN to be limited to 256 characters (typical = 180-200)
  uint16 checkSum;      // USED BY ConfigStore!
} ConfigData;

