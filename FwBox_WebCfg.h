//
// Copyright (c) 2022 Fw-Box (https://fw-box.com)
// Author: Hartman Hsieh
//
// Description :
//   
//
// Connections :
//   
//
// Required Library :
//   PubSubClient
//   https://github.com/fw-box/FwBox_Preferences
//

#ifndef __FWBOX_WEB_CFG_H__
#define __FWBOX_WEB_CFG_H__

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <Update.h>
  //#include <Preferences.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266httpUpdate.h>
  #include <ESP8266WebServer.h>
  //#include <LittleFS.h>
  //#define ICACHE_RODATA_ATTR  __attribute__((section(".irom.text")))
  //#define PROGMEM ICACHE_RODATA_ATTR
#endif
#include <PubSubClient.h>
#include "FwBox_Preferences.h"

#define ITEM_COUNT 5
#define ITEM_TYPE_STRING 1
#define ITEM_TYPE_INT 2
#define ITEM_TYPE_EN_DIS 3

class FwBox_WebCfg
{

public:
    FwBox_WebCfg();

    //
    // return :
    //          0 - Success
    //          1 - Failed
    //
    int begin();
    int earlyBegin();

    void handle();
    String getMac();
    static void handleRoot();

    //const char HTML_CFG_SERVER[] PROGMEM = "<!DOCTYPE html><html><head><title>Module</title><style>td{font-size:30px;padding:10px;} input{font-size:30px;padding:10px;}</style></head><body><center><h1>Chily Module</h1><form action='/' method='post'><table><tr><td>WiFi SSID</td></tr><tr><td><input type='text' name='ssid'></td></tr><tr><td>WiFi Password</td></tr><tr><td><input type='password' name='pw'></td></tr><tr><td>Server IP</td></tr><tr><td><input type='text' name='ip'></td></tr><tr><td colspan='2'><input type='submit'></td></tr></table></form></center></body></html>";
#if defined(ESP32)
    static WebServer SettingServer;
#else
    static ESP8266WebServer SettingServer;
#endif
    static bool SettingServerRunning;

    static String MqttBrokerIp;

    static String ItemName[ITEM_COUNT];
    static String ItemKey[ITEM_COUNT];
    static int ItemType[ITEM_COUNT];

    void setItem(int idx, String name, String itemKey);
    void setItem(int idx, String name, String itemKey, int itemType);
    String getItemValueString(const char* key);
    int getItemValueInt(const char* key, const int32_t defaultValue);

    void configFileBegin();
    String configFileGetString(const char* key);
    void configFileEnd();

private:
    String WifiSsid = "";
    String WifiPassword = "";
    bool EarlyBeginRun = false;
    unsigned long LastWifiConnectTime = 0;
};



#endif // __FWBOX_WEB_CFG_H__
