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
//

#include "FwBox_WebCfg.h"


WebServer FwBox_WebCfg::SettingServer;
bool FwBox_WebCfg::SettingServerRunning = false;
String FwBox_WebCfg::MqttBrokerIp = "";
String FwBox_WebCfg::ItemName[ITEM_COUNT];
String FwBox_WebCfg::ItemKey[ITEM_COUNT];
int FwBox_WebCfg::ItemType[ITEM_COUNT];

FwBox_WebCfg::FwBox_WebCfg()
{
    for (int ii = 0; ii < ITEM_COUNT; ii++) {
        FwBox_WebCfg::ItemName[ii] = "";
        FwBox_WebCfg::ItemKey[ii] = "";
        FwBox_WebCfg::ItemType[ii] = -1;
    }
}

int FwBox_WebCfg::begin()
{
    FwBox_WebCfg::SettingServerRunning = false;
    String str_ssid = "";
    String str_pass = "";

    Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    str_ssid = Prefs.getString("WIFI_SSID");
    str_pass = Prefs.getString("WIFI_PW");
    Prefs.end();

    String s_id = getMac().substring(8);
    s_id = "FW-BOX_" + s_id;

    WiFi.mode(WIFI_AP_STA);
    if (str_ssid.length() > 0) {
        WiFi.begin(str_ssid.c_str(), str_pass.c_str());

        for (int ii = 0; ii < 50; ii++) {
            if (WiFi.status() == WL_CONNECTED)
                break;
            delay(500);
            Serial.print(".");
        }

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else {
        WiFi.softAP(s_id.c_str(), "");
    }

    FwBox_WebCfg::SettingServer.on("/", handleRoot);

    FwBox_WebCfg::SettingServer.on("/update", HTTP_POST, []() {
      FwBox_WebCfg::SettingServer.sendHeader("Connection", "close");
      FwBox_WebCfg::SettingServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = FwBox_WebCfg::SettingServer.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin()) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      } else {
        Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
      }
    });

    FwBox_WebCfg::SettingServer.begin();
    Serial.println("HTTP server started");

    FwBox_WebCfg::SettingServerRunning = true;

    return 1; // Error
}

void FwBox_WebCfg::handle()
{
  if (FwBox_WebCfg::SettingServerRunning == true) {
    FwBox_WebCfg::SettingServer.handleClient();
  }
}

void FwBox_WebCfg::handleRoot()
{
    String str_ssid = "";
    String str_pass = "";
    //String str_ip = "";
    bool user_input = false;
    Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    for (uint8_t ai = 0; ai < FwBox_WebCfg::SettingServer.args(); ai++) {
        if (FwBox_WebCfg::SettingServer.argName(ai).equals("ssid") == true) {
            str_ssid = FwBox_WebCfg::SettingServer.arg(ai);
            user_input = true;
        }
        else if (FwBox_WebCfg::SettingServer.argName(ai).equals("pw") == true) {
            str_pass = SettingServer.arg(ai);
            user_input = true;
        }

        for (int ii = 0; ii < ITEM_COUNT; ii++) {
            if (FwBox_WebCfg::ItemName[ii].length() > 0 && FwBox_WebCfg::ItemKey[ii].length() > 0 && FwBox_WebCfg::ItemType[ii] > 0) {
                if (FwBox_WebCfg::SettingServer.argName(ai).equals(FwBox_WebCfg::ItemKey[ii]) == true) {
                    if (FwBox_WebCfg::ItemType[ii] == ITEM_TYPE_INT || FwBox_WebCfg::ItemType[ii] == ITEM_TYPE_EN_DIS) {
                        if (FwBox_WebCfg::SettingServer.arg(ai).length() > 0)
                            Prefs.putInt(FwBox_WebCfg::ItemKey[ii].c_str(), FwBox_WebCfg::SettingServer.arg(ai).toInt());
                        else
                            Prefs.remove(FwBox_WebCfg::ItemKey[ii].c_str());
                    }
                    else {
                        Prefs.putString(FwBox_WebCfg::ItemKey[ii].c_str(), FwBox_WebCfg::SettingServer.arg(ai));
                    }
                    user_input = true;
                }
            }
            else {
                break; // The item index must be 0,1,2...
            }
        }
    }
    Prefs.end();

    if (user_input == true) {
        Serial.print("WEB SSID = ");
        Serial.println(str_ssid);
        Serial.print("WEB PASSWORD = ");
        Serial.println(str_pass);
        //Preferences Prefs;
        Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
        /*if (str_ip.length() > 0) {
            FwBox_WebCfg::MqttBrokerIp = str_ip;
            Prefs.putString("IP", FwBox_WebCfg::MqttBrokerIp.c_str());
        }*/
        if (str_ssid.length() > 0) {
            Prefs.putString("WIFI_SSID", str_ssid.c_str());
            Prefs.putString("WIFI_PW", str_pass.c_str());
        }
        Prefs.end();
    }

    String str_item_list = "";
    //Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    str_ssid = Prefs.getString("WIFI_SSID");
    str_pass = Prefs.getString("WIFI_PW");
    //str_ip = Prefs.getString("IP");
    for (int ii = 0; ii < ITEM_COUNT; ii++) {
        if (FwBox_WebCfg::ItemName[ii].length() > 0 && FwBox_WebCfg::ItemKey[ii].length() > 0 && FwBox_WebCfg::ItemType[ii] > 0) {
            if (FwBox_WebCfg::ItemType[ii] == ITEM_TYPE_INT) {
                int i_temp = Prefs.getInt(FwBox_WebCfg::ItemKey[ii].c_str(), -1);
                Serial.printf("%s = %d\n", FwBox_WebCfg::ItemKey[ii].c_str(), i_temp);
                str_item_list += "<p><div class='it'>"+FwBox_WebCfg::ItemName[ii]+"<div><input type='text' name='"+FwBox_WebCfg::ItemKey[ii]+"' value='"+i_temp+"'></div></div></p>";
            }
            else if (FwBox_WebCfg::ItemType[ii] == ITEM_TYPE_EN_DIS) {
                int i_temp = Prefs.getInt(FwBox_WebCfg::ItemKey[ii].c_str(), -1);
                Serial.printf("%s = %d\n", FwBox_WebCfg::ItemKey[ii].c_str(), i_temp);
                String str_en = "";
                String str_dis = "";
                if (i_temp == 0)
                    str_dis = " selected";
                else if (i_temp > 0)
                    str_en = " selected";
                str_item_list += "<p><div class='it'>"+FwBox_WebCfg::ItemName[ii]+"<div><select name='"+FwBox_WebCfg::ItemKey[ii]+"' value='"+i_temp+"'><option value=1"+str_en+">Enable</option><option value=0"+str_dis+">Disable</option></select></div></div></p>";
            }
            else {
                String str_temp = Prefs.getString(FwBox_WebCfg::ItemKey[ii].c_str());
                str_item_list += "<p><div class='it'>"+FwBox_WebCfg::ItemName[ii]+"<div><input type='text' name='"+FwBox_WebCfg::ItemKey[ii]+"' value='"+str_temp+"'></div></div></p>";
            }
        }
        else {
            break; // The item index must be 0,1,2...
        }
    }
    Prefs.end();

    char temp[1024+512];
    char html_cfg[] PROGMEM = 
"<!DOCTYPE html>\
<html>\
<head>\
<title>ESP32 Demo</title>\
<style>\
body{font-family:Arial;}\
form{width:90%;}\
p{width:100%;text-align:center;}\
div{width:100%;font-size:4.5vw;text-align:center;}\
input{font-size:4.5vw;}\
select{font-size:4.5vw;}\
option{font-size:4.5vw;}\
.it{background-color:#40E0D0;padding:1vw 0 1vw 0;}\
.it2{background-color:#1E90FF;padding:1vw 0 1vw 0;}\
</style>\
</head>\
<body>\
<center>\
<form action='/' method='post'>\
<h1 style='font-size:6vw;'>WebCfg</h1>\
<p><div class='it'>WiFi SSID<div><input type='text' name='ssid' value='%s'></div></div></p>\
<p><div class='it'>WiFi Password<div><input type='password' name='pw' value='%s'></div></div></p>\
%s\
<p style='text-align:end;'><input type='submit' value=' Submit '></p>\
</form><br>\
<p><div class='it2'>Firmware Update<div><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form></div></div></p>\
<p style='font-size:4.5vw;text-align:end;'><a href='https://fw-box.com'>https://fw-box.com</a></p>\
</center>\
</body>\
</html>";
    sprintf(temp, html_cfg, str_ssid.c_str(), str_pass.c_str(), str_item_list.c_str());

    Serial.printf("HTML data length = %d\n", strlen(temp));
    FwBox_WebCfg::SettingServer.send(200, "text/html", temp);

    if (user_input == true) {
        Serial.println("Restarting...");
        delay(500);
        ESP.restart();
        while (1)
            delay(100);
    }
}

void FwBox_WebCfg::setItem(int idx, String name, String itemKey)
{
    if (idx < 0 && idx >= ITEM_COUNT)
        return;

    FwBox_WebCfg::ItemName[idx] = name;
    FwBox_WebCfg::ItemKey[idx] = itemKey;
    FwBox_WebCfg::ItemType[idx] = ITEM_TYPE_STRING;
}

void FwBox_WebCfg::setItem(int idx, String name, String itemKey, int itemType)
{
    if (idx < 0 && idx >= ITEM_COUNT)
        return;

    FwBox_WebCfg::ItemName[idx] = name;
    FwBox_WebCfg::ItemKey[idx] = itemKey;
    FwBox_WebCfg::ItemType[idx] = itemType;
}

String FwBox_WebCfg::getItemValueString(const char* key)
{
    String str_temp = "";
    Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    str_temp = Prefs.getString(key);
    Prefs.end();
    return str_temp;
}

int FwBox_WebCfg::getItemValueInt(const char* key, const int32_t defaultValue)
{
    int i_temp = defaultValue;
    Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    i_temp = Prefs.getInt(key, defaultValue);
    Prefs.end();
    return i_temp;
}

String FwBox_WebCfg::getMac()
{
  String str_mac = WiFi.macAddress();
  str_mac.replace(":", "");
  return str_mac;
}
