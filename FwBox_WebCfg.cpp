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

#include "FwBox_WebCfg.h"

#if defined(ESP32)
    WebServer FwBox_WebCfg::SettingServer;
#else
    ESP8266WebServer FwBox_WebCfg::SettingServer;
#endif
bool FwBox_WebCfg::SettingServerRunning = false;
String FwBox_WebCfg::MqttBrokerIp = "";
String FwBox_WebCfg::ItemName[ITEM_COUNT];
String FwBox_WebCfg::ItemKey[ITEM_COUNT];
String FwBox_WebCfg::ItemDescription[ITEM_COUNT];
int FwBox_WebCfg::ItemType[ITEM_COUNT];
int FwBox_WebCfg::ButtonClickItemIndex;

FwBox_WebCfg::FwBox_WebCfg ()
{
    for (int ii = 0; ii < ITEM_COUNT; ii++) {
        FwBox_WebCfg::ItemName[ii] = "";
        FwBox_WebCfg::ItemKey[ii] = "";
        FwBox_WebCfg::ItemDescription[ii] = "";
        FwBox_WebCfg::ItemType[ii] = -1;
    }
    FwBox_WebCfg::ButtonClickItemIndex = -1;
}

int FwBox_WebCfg::earlyBegin ()
{
    FwBox_Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    //Prefs.dumpPairList();
    WifiSsid = Prefs.getString("WIFI_SSID");
    WifiPassword = Prefs.getString("WIFI_PW");
    Prefs.end();

    Serial.println("earlyBegin () :");

    Serial.printf("WIFI_SSID : %s\n", WifiSsid.c_str());
    Serial.printf("WIFI_PW : %s\n", WifiPassword.c_str());

    WiFi.mode(WIFI_AP_STA);
    FwBox_WebCfg::LastWifiConnectTime = millis();
    if (WifiSsid.length() > 0) {
        WiFi.begin(WifiSsid.c_str(), WifiPassword.c_str());
    }
    else {
        startSoftAp ();
    }

    FwBox_WebCfg::EarlyBeginRun = true;

    return 0; // 0 - Success
}

int FwBox_WebCfg::begin ()
{
    FwBox_WebCfg::SettingServerRunning = false;

    Serial.println("");
    Serial.println("begin () :");
    Serial.print("FwBox_WebCfg::EarlyBeginRun = ");
    Serial.println(FwBox_WebCfg::EarlyBeginRun);

    if (FwBox_WebCfg::EarlyBeginRun == true) {
        if (WifiSsid.length () > 0) {
            waitForWifiConnectingAndPrintInfo ();
        }
    }
    else {
        FwBox_Preferences Prefs;
        Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
        //Prefs.dumpPairList();
        WifiSsid = Prefs.getString("WIFI_SSID");
        WifiPassword = Prefs.getString("WIFI_PW");
        Prefs.end();

        Serial.printf("WIFI_SSID : %s\n", WifiSsid.c_str());
        Serial.printf("WIFI_PW : %s\n", WifiPassword.c_str());

        WiFi.mode(WIFI_AP_STA);
        FwBox_WebCfg::LastWifiConnectTime = millis();
        if (WifiSsid.length() > 0) {
            WiFi.begin(WifiSsid.c_str(), WifiPassword.c_str());
            waitForWifiConnectingAndPrintInfo ();
        }
        else {
            startSoftAp ();
        }
    }

    FwBox_WebCfg::SettingServer.on("/", handleRoot);
    FwBox_WebCfg::SettingServer.on("/btn_clk", handleRoot);

#if defined(ESP32)
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
#else
    FwBox_WebCfg::SettingServer.on("/update", HTTP_POST, []() {
      FwBox_WebCfg::SettingServer.sendHeader("Connection", "close");
      FwBox_WebCfg::SettingServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = FwBox_WebCfg::SettingServer.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
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
      }
      yield();
    });
#endif

    FwBox_WebCfg::SettingServer.begin();
    Serial.println("HTTP server started");

    FwBox_WebCfg::SettingServerRunning = true;

    return 1; // Error
}

void FwBox_WebCfg::handle ()
{
  
    if (FwBox_WebCfg::SettingServerRunning == true) {
        FwBox_WebCfg::SettingServer.handleClient();
    }

    if (WiFi.status() != WL_CONNECTED) {
        //
        // If WiFi is disconnected, retry to connect WiFi every 6 minutes.
        //
        if (millis() - FwBox_WebCfg::LastWifiConnectTime > (6 * 60 * 1000)) {
            Serial.println("handle () :");
            FwBox_WebCfg::LastWifiConnectTime = millis();
            WiFi.mode(WIFI_AP_STA);
            if (WifiSsid.length() > 0) {
                
                Serial.print("WiFi SSID = ");
                Serial.println(WifiSsid);
                Serial.print("WiFi PASSWORD = ");
                Serial.println(WifiPassword);
                WiFi.begin(WifiSsid.c_str(), WifiPassword.c_str());
            }
            else {
                String s_id = getMac().substring(8);
                String str_middle = FwBox_WebCfg::WiFiApMiddleName;
                if (str_middle.length() > 0) {
                    str_middle.toUpperCase();
                    s_id = "FW-BOX_" + str_middle + "_" + s_id;
                }
                else
                    s_id = "FW-BOX_" + s_id;
                Serial.printf("WIFI AP NAME (handle) : %s\n", s_id.c_str());
                WiFi.softAP(s_id.c_str(), "");
            }
        }
    }
}

void FwBox_WebCfg::handleRoot ()
{
    String str_ssid_user_input = "";
    String str_pass_user_input = "";
    String str_ssid = "";
    String str_pass = "";
    //String str_ip = "";
    bool user_input_wifi = false;
    bool user_input = false;
    FwBox_Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    for (uint8_t ai = 0; ai < FwBox_WebCfg::SettingServer.args(); ai++) {
        //
        // Check user's input of Wifi SSID and password.
        //
        if (FwBox_WebCfg::SettingServer.argName(ai).equals("ssid") == true) {
            str_ssid_user_input = FwBox_WebCfg::SettingServer.arg(ai);
            user_input_wifi = true;
        }
        else if (FwBox_WebCfg::SettingServer.argName(ai).equals("pw") == true) {
            str_pass_user_input = SettingServer.arg(ai);
            user_input_wifi = true;
        }
        else if (FwBox_WebCfg::SettingServer.argName(ai).equals("item_index") == true) {
            FwBox_WebCfg::ButtonClickItemIndex = SettingServer.arg(ai).toInt();
            Serial.println("item_index = " + FwBox_WebCfg::ButtonClickItemIndex);
        }

        //
        // Check user's input of items and save them to storage.
        //
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

    //
    // Read WIfi SSID and password from storage.
    //
    str_ssid = Prefs.getString("WIFI_SSID");
    str_pass = Prefs.getString("WIFI_PW");
    Prefs.end();

    if (user_input_wifi == true) {
        Serial.println ("User iput :");
        Serial.print("WiFi SSID = ");
        Serial.println(str_ssid_user_input);
        Serial.print("WiFi PASSWORD = ");
        Serial.println(str_pass_user_input);
        
        //
        // Only save user input SSID and password when they are different from storage's.
        //
        if (str_ssid_user_input.equals (str_ssid) == false || str_pass_user_input.equals (str_pass) == false) {
            Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
            if (str_ssid_user_input.equals (str_ssid) == false)
                Prefs.putString("WIFI_SSID", str_ssid_user_input.c_str());
            if (str_pass_user_input.equals (str_pass) == false)
                Prefs.putString("WIFI_PW", str_pass_user_input.c_str());
            Prefs.end();
        }
        else {
            user_input_wifi = false; // SSID and password doesn't change. Set the variable to false.
        }
    }

    String str_item_list = "";
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
            else if (FwBox_WebCfg::ItemType[ii] == ITEM_TYPE_BUTTON) {
                Serial.println("ITEM_TYPE_BUTTON");
                str_item_list += "<p><div class='it'>"+FwBox_WebCfg::ItemDescription[ii]+"<div><button onclick=\"window.location.href='/btn_clk?item_index="+ii+"';\">"+FwBox_WebCfg::ItemName[ii]+"</button></div></div></p>";
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

    //
    // Edit the HTML of web page
    //
    static const char html_cfg_a_0[] PROGMEM = "<!DOCTYPE html>\
<html>\
<head>\
<title>FW-BOX : WebCfg</title>\
<style>";
    static const char html_cfg_a_1[] PROGMEM = "<!DOCTYPE html>\
body{font-family:Arial;}\
form{width:90%%;}\
p{width:100%%;text-align:center;}\
div{width:100%%;font-size:4.5vw;text-align:center;}\
input{font-size:4.5vw;}\
select{font-size:4.5vw;}\
option{font-size:4.5vw;}\
.it{background-color:#40E0D0;padding:1vw 0 1vw 0;}\
.it2{background-color:#1E90FF;padding:1vw 0 1vw 0;}";
    static const char html_cfg_a_2[] PROGMEM = "<!DOCTYPE html>\
</style>\
</head>\
<body>\
<center>\
<form action='/' method='post'>\
<h1 style='font-size:6vw;'>WebCfg</h1>\
<p><div class='it'>WiFi SSID<div><input type='text' name='ssid' value='";
    static const char html_cfg_b[] PROGMEM = "'></div></div></p>\
<p><div class='it'>WiFi Password<div><input type='password' name='pw' value='";
    static const char html_cfg_c[] PROGMEM = "'></div></div></p>";
    static const char html_cfg_d[] PROGMEM = "\
<p style='text-align:end;'><input type='submit' value=' Submit '></p>\
</form><br>";
    static const char html_cfg_e[] PROGMEM = "<h2>Saved</h2>";
    static const char html_cfg_f[] PROGMEM = "\
<p><div class='it2'>Firmware Update<div><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form></div></div></p>\
<p style='font-size:4.5vw;text-align:end;'><a href='https://fw-box.com'>https://fw-box.com</a></p>\
</center>\
</body>\
</html>";

    //
    // Calculate the HTML content length
    //
    int content_len = strlen(html_cfg_a_0);
    content_len += strlen(html_cfg_a_1);
    content_len += strlen(html_cfg_a_2);
    content_len += strlen(str_ssid.c_str());
    content_len += strlen(html_cfg_b);
    content_len += strlen(str_pass.c_str());
    content_len += strlen(html_cfg_c);
    content_len += strlen(str_item_list.c_str());
    content_len += strlen(html_cfg_d);
    if (user_input == true)
        content_len += strlen(html_cfg_e);
    content_len += strlen(html_cfg_f);

    //
    // Send the HTML content
    //
    FwBox_WebCfg::SettingServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    FwBox_WebCfg::SettingServer.sendHeader("Pragma", "no-cache");
    FwBox_WebCfg::SettingServer.sendHeader("Expires", "-1");
    FwBox_WebCfg::SettingServer.setContentLength(content_len);
    // here begin chunked transfer
    FwBox_WebCfg::SettingServer.send(200, "text/html", "");
    FwBox_WebCfg::SettingServer.sendContent(html_cfg_a_0);
    FwBox_WebCfg::SettingServer.sendContent(html_cfg_a_1);
    FwBox_WebCfg::SettingServer.sendContent(html_cfg_a_2);
    FwBox_WebCfg::SettingServer.sendContent(str_ssid.c_str());
    FwBox_WebCfg::SettingServer.sendContent(html_cfg_b);
    FwBox_WebCfg::SettingServer.sendContent(str_pass.c_str());
    FwBox_WebCfg::SettingServer.sendContent(html_cfg_c);
    FwBox_WebCfg::SettingServer.sendContent(str_item_list.c_str());
    FwBox_WebCfg::SettingServer.sendContent(html_cfg_d);
    if (user_input == true)
        FwBox_WebCfg::SettingServer.sendContent(html_cfg_e);
    FwBox_WebCfg::SettingServer.sendContent(html_cfg_f);
    FwBox_WebCfg::SettingServer.sendContent(F("")); // this tells web client that transfer is done
    FwBox_WebCfg::SettingServer.client().stop();

    if (user_input_wifi == true) { // When user inputs SSID and password and they are saved into storage. Restart ESP.
        Serial.println("Restarting...");
        delay(5000);
        ESP.restart();
        while (1)
            delay(100);
    }
}

void FwBox_WebCfg::setWiFiApMiddleName (const char* apMiddleName)
{
    FwBox_WebCfg::WiFiApMiddleName = apMiddleName;
}

void FwBox_WebCfg::setItem (int index, String name, String itemKey)
{
    if (index < 0 && index >= ITEM_COUNT)
        return;

    FwBox_WebCfg::ItemName[index] = name;
    FwBox_WebCfg::ItemKey[index] = itemKey;
    FwBox_WebCfg::ItemType[index] = ITEM_TYPE_STRING;
}

void FwBox_WebCfg::setItem (int index, String name, String itemKey, int itemType)
{
    if (index < 0 && index >= ITEM_COUNT)
        return;

    FwBox_WebCfg::ItemName[index] = name;
    FwBox_WebCfg::ItemKey[index] = itemKey;
    FwBox_WebCfg::ItemType[index] = itemType;
}

void FwBox_WebCfg::setItem (int index, String name, String itemKey, String itemDescription, int itemType)
{
    if (index < 0 && index >= ITEM_COUNT)
        return;

    FwBox_WebCfg::ItemName[index] = name;
    FwBox_WebCfg::ItemKey[index] = itemKey;
    FwBox_WebCfg::ItemDescription[index] = itemDescription;
    FwBox_WebCfg::ItemType[index] = itemType;
}

String FwBox_WebCfg::getItemValueString (const char* key)
{
    String str_temp = "";
    FwBox_Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    str_temp = Prefs.getString(key);
    Prefs.end();
    return str_temp;
}

int FwBox_WebCfg::getItemValueInt (const char* key, const int32_t defaultValue)
{
    int i_temp = defaultValue;
    FwBox_Preferences Prefs;
    Prefs.begin("FW-BOX"); // use "FW-BOX" namespace
    i_temp = Prefs.getInt(key, defaultValue);
    Prefs.end();
    return i_temp;
}

String FwBox_WebCfg::getMac ()
{
  String str_mac = WiFi.macAddress();
  str_mac.replace(":", "");
  return str_mac;
}

void FwBox_WebCfg::startSoftAp ()
{
    String s_id = getMac().substring(8);
    String str_middle = FwBox_WebCfg::WiFiApMiddleName;
    if (str_middle.length() > 0) {
        str_middle.toUpperCase();
        s_id = "FW-BOX_" + str_middle + "_" + s_id;
    }
    else
        s_id = "FW-BOX_" + s_id;
    Serial.printf("WIFI AP NAME (begin) : %s\n", s_id.c_str());
    WiFi.softAP(s_id.c_str(), "");
}

void FwBox_WebCfg::waitForWifiConnectingAndPrintInfo ()
{
    for (int ii = 0; ii < 50; ii++) {
        if (WiFi.status() == WL_CONNECTED)
            break;
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    if (WiFi.status () == WL_CONNECTED) {
        Serial.println ("WiFi connected");
        Serial.println ("IP address: ");
        Serial.println (WiFi.localIP());
    }
    else {
        Serial.println ("WiFi disconnected");
        startSoftAp ();
    }
}