//
// Copyright (c) 2020 Fw-Box (https://fw-box.com)
// Author: Hartman Hsieh
//
// Description :
//   None
//
// Connections :
//
// Required Library :
//   https://github.com/plapointe6/HAMqttDevice
//

#include <Wire.h>
#include "FwBox_PMSX003.h"
#include "SoftwareSerial.h"
#include <U8g2lib.h>
#include "FwBox_WebCfg.h"
#include <PubSubClient.h>
#include "HAMqttDevice.h"

#define DEVICE_TYPE 11
#define FIRMWARE_VERSION "1.0.0"
#define VALUE_COUNT 5

#define ANALOG_VALUE_DEBOUNCING 8

//
// Debug definitions
//
#define FW_BOX_DEBUG 1

#if FW_BOX_DEBUG == 1
  #define DBG_PRINT(VAL) Serial.print(VAL)
  #define DBG_PRINTLN(VAL) Serial.println(VAL)
  #define DBG_PRINTF(FORMAT, ARG) Serial.printf(FORMAT, ARG)
  #define DBG_PRINTF2(FORMAT, ARG1, ARG2) Serial.printf(FORMAT, ARG1, ARG2)
#else
  #define DBG_PRINT(VAL)
  #define DBG_PRINTLN(VAL)
  #define DBG_PRINTF(FORMAT, ARG)
  #define DBG_PRINTF2(FORMAT, ARG1, ARG2)
#endif

#define VAL_PM1_0 0
#define VAL_PM2_5 1
#define VAL_PM10_0 2
#define VAL_TEMP 3
#define VAL_HUMI 4

//
// WebCfg instance
//
FwBox_WebCfg WebCfg;

WiFiClient espClient;
PubSubClient MqttClient(espClient);
String MqttBrokerIp = "";
String MqttBrokerUsername = "";
String MqttBrokerPassword = "";
String DevName = "";

HAMqttDevice* HaDev = 0;
bool HAEnable = false;

//
// PMS5003T, PMS5003, PM3003
//
SoftwareSerial SerialSensor(D7, D8); // RX:D7, TX:D8
FwBox_PMSX003 Pms(&SerialSensor);

//
// Set the unit of the values before "display".
//
// ValUnit[0] = "μg/m³";
// ValUnit[1] = "μg/m³";
// ValUnit[2] = "μg/m³";
// ValUnit[3] = "°C";
// ValUnit[4] = "%";
//
String ValUnit[VALUE_COUNT] = {"μg/m³", "μg/m³", "μg/m³", "°C", "%"};
float Value[VALUE_COUNT] = {0.0,0.0,0.0,0.0,0.0};

unsigned long ReadingTime = 0;

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  WebCfg.earlyBegin();

  pinMode(LED_BUILTIN, OUTPUT);

  //
  // Initialize the PMSX003 Sensor
  //
  Pms.begin();
  
  //
  // Create 5 inputs in web page.
  //
  WebCfg.setItem(0, "MQTT Broker IP", "MQTT_IP"); // string input
  WebCfg.setItem(1, "MQTT Broker Username", "MQTT_USER"); // string input
  WebCfg.setItem(2, "MQTT Broker Password", "MQTT_PASS"); // string input
  WebCfg.setItem(3, "Home Assistant", "HA_EN_DIS", ITEM_TYPE_EN_DIS); // enable/disable select input
  WebCfg.setItem(4, "Device Name", "DEV_NAME"); // string input
  WebCfg.begin();

  //
  // Get the value of "MQTT_IP" from web input.
  //
  MqttBrokerIp = WebCfg.getItemValueString("MQTT_IP");
  Serial.printf("MQTT Broker IP = %s\n", MqttBrokerIp.c_str());

  MqttBrokerUsername = WebCfg.getItemValueString("MQTT_USER");
  Serial.printf("MQTT Broker Username = %s\n", MqttBrokerUsername.c_str());

  MqttBrokerPassword = WebCfg.getItemValueString("MQTT_PASS");
  Serial.printf("MQTT Broker Password = %s\n", MqttBrokerPassword.c_str());

  DevName = WebCfg.getItemValueString("DEV_NAME");
  Serial.printf("Device Name = %s\n", DevName.c_str());
  if (DevName.length() <= 0) {
    DevName = "box_dust_sensor_";
    String str_mac = WiFi.macAddress();
    str_mac.replace(":", "");
    str_mac.toLowerCase();
    if (str_mac.length() >= 12) {
      DevName = DevName + str_mac.substring(8);
      Serial.printf("Auto generated Device Name = %s\n", DevName.c_str());
    }
  }

  if (WebCfg.getItemValueInt("HA_EN_DIS", 0) == 1) {
    HAEnable = true;
    HaDev = new HAMqttDevice(DevName, HAMqttDevice::SENSOR, "homeassistant");

    //HaDev->enableCommandTopic();
    //HaDev->enableAttributesTopic();

    //HaDev->addConfigVar("device_class", "door")
    //HaDev->addConfigVar("retain", "false");

    Serial.println("HA Config topic : " + HaDev->getConfigTopic());
    Serial.println("HA Config payload : " + HaDev->getConfigPayload());
    Serial.println("HA State topic : " + HaDev->getStateTopic());
    Serial.println("HA Command topic : " + HaDev->getCommandTopic());
    Serial.println("HA Attributes topic : " + HaDev->getAttributesTopic());

    if (WiFi.status() == WL_CONNECTED) {
      HaMqttConnect(MqttBrokerIp, MqttBrokerUsername, MqttBrokerPassword, HaDev->getConfigTopic(), HaDev->getConfigPayload());
    }
  }
  Serial.printf("Home Assistant = %d\n", HAEnable);

} // void setup()

void loop()
{
  WebCfg.handle();
  
  if((millis() - ReadingTime) > 5000) { // Read and send the sensor data every 5 seconds.
    //
    // Read the sensors
    //
    if(readSensor() == 0) { // Success
      DBG_PRINTLN("Success to read sensor data.");

      Value[VAL_PM1_0] = Pms.pm1_0();
      Value[VAL_PM2_5] = Pms.pm2_5();
      Value[VAL_PM10_0] = Pms.pm10_0();
      Value[VAL_TEMP] = Pms.temp();
      Value[VAL_HUMI] = Pms.humi();

      DBG_PRINTF("WL_CONNECTED=%d\n", WL_CONNECTED);
      DBG_PRINTF2("WiFi.status()=%d, MqttClient.connected()=%d\n", WiFi.status(), MqttClient.connected());

      if (WiFi.status() == WL_CONNECTED && !MqttClient.connected()) {
        HaMqttConnect(MqttBrokerIp, MqttBrokerUsername, MqttBrokerPassword, HaDev->getConfigTopic(), HaDev->getConfigPayload());
      }
      
      if (WiFi.status() == WL_CONNECTED && MqttClient.connected()) {
        String str_payload = "{";
        str_payload +=  "\"pm1_0\":";
        str_payload += (int)Value[VAL_PM1_0];
        str_payload += ",\"pm2_5\":";
        str_payload += (int)Value[VAL_PM2_5];
        str_payload += ",\"pm10_0\":";
        str_payload += (int)Value[VAL_PM10_0];
        if(Pms.readDeviceType() == FwBox_PMSX003::PMS5003T) {
          str_payload += ",\"temp\":";
          str_payload += (int)Value[VAL_TEMP];
          str_payload += ",\"humi\":";
          str_payload += (int)Value[VAL_HUMI];
        }
        str_payload += "}";
        DBG_PRINTLN(HaDev->getStateTopic());
        DBG_PRINTLN(str_payload);
        bool result_publish = MqttClient.publish(HaDev->getStateTopic().c_str(), str_payload.c_str());
        DBG_PRINTF("result_publish=%d\n", result_publish);
      }
    }

    ReadingTime = millis();
  }

} // END OF "void loop()"

uint8_t readSensor()
{
  //
  // Running readPms before running pm2_5, temp, humi and readDeviceType.
  //
  if(Pms.readPms()) {
    if(Pms.readDeviceType() == FwBox_PMSX003::PMS5003T) {
      DBG_PRINTLN("PMS5003T is detected.");
      if((Pms.pm1_0() == 0) && (Pms.pm2_5() == 0) && (Pms.pm10_0() == 0) && (Pms.temp() == 0) && (Pms.humi() == 0)) {
        DBG_PRINTLN("PMS data format is wrong.");
      }
      else {
        DBG_PRINT("PM1.0=");
        DBG_PRINTLN(Pms.pm1_0());
        DBG_PRINT("PM2.5=");
        DBG_PRINTLN(Pms.pm2_5());
        DBG_PRINT("PM10=");
        DBG_PRINTLN(Pms.pm10_0());
        DBG_PRINT("Temperature=");
        DBG_PRINTLN(Pms.temp());
        DBG_PRINT("Humidity=");
        DBG_PRINTLN(Pms.humi());
        return 0; // Success
      }
    }
    else if(Pms.readDeviceType() == FwBox_PMSX003::PMS5003) {
      DBG_PRINTLN("PMS5003 is detected.");
      if((Pms.pm1_0() == 0) && (Pms.pm2_5() == 0) && (Pms.pm10_0() == 0)) {
        DBG_PRINTLN("PMS data format is wrong.");
      }
      else {
        DBG_PRINT("PM1.0=");
        DBG_PRINTLN(Pms.pm1_0());
        DBG_PRINT("PM2.5=");
        DBG_PRINTLN(Pms.pm2_5());
        DBG_PRINT("PM10=");
        DBG_PRINTLN(Pms.pm10_0());
        return 0; // Success
      }
    }
    else if(Pms.readDeviceType() == FwBox_PMSX003::PMS3003) {
      DBG_PRINTLN("PMS3003 is detected.");
      if((Pms.pm1_0() == 0) && (Pms.pm2_5() == 0) && (Pms.pm10_0() == 0)) {
        DBG_PRINTLN("PMS data format is wrong.");
      }
      else {
        DBG_PRINT("PM1.0=");
        DBG_PRINTLN(Pms.pm1_0());
        DBG_PRINT("PM2.5=");
        DBG_PRINTLN(Pms.pm2_5());
        DBG_PRINT("PM10=");
        DBG_PRINTLN(Pms.pm10_0());
        return 0; // Success
      }
    }
  }
  else {
    DBG_PRINTLN("Failed to read PSMX003Y.");
  }

  return 1; // Error
}

void HaMqttConnect(const String& brokerIp, const String& brokerUsername, const String& brokerPassword, const String& configTopic, const String& configPayload)
{
  if (brokerIp.length() > 0) {
    MqttClient.setServer(brokerIp.c_str(), 1883);
    MqttClient.setCallback(MqttCallback);

    if (!MqttClient.connected()) {
      Serial.print("Attempting MQTT connection...\n");
      Serial.print("MQTT Broker Ip : ");
      Serial.println(brokerIp);
      // Create a random client ID
      String str_mac = WiFi.macAddress();
      str_mac.replace(":", "");
      str_mac.toUpperCase();
      String client_id = "Fw-Box-";
      client_id += str_mac;
      Serial.println("client_id :" + client_id);
      // Attempt to connect
      if (MqttClient.connect(client_id.c_str(), brokerUsername.c_str(), brokerPassword.c_str())) {
        Serial.println("connected");
      } else {
        Serial.print("failed, rc=");
        Serial.print(MqttClient.state());
        Serial.println(" try again in 5 seconds");
      }
    }

    if (MqttClient.connected()) {
      Serial.printf("configTopic.c_str()=%s\n", configTopic.c_str());
      Serial.printf("configPayload.c_str()=%s\n", configPayload.c_str());
      bool result_publish = MqttClient.publish(configTopic.c_str(), configPayload.c_str());
      DBG_PRINTF("result_publish=%d\n", result_publish);
    }
  } // END OF "if (brokerIp.length() > 0)"
}
/*
//
// Callback function for MQTT subscribe.
//
void MqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
*/
