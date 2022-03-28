# FwBox_WebCfg
ESP32 web config library. Config WiFi, custom items and web OTA.

# Example : Integer input
```cpp
#include "FwBox_WebCfg.h"
#include "DHTesp.h"

FwBox_WebCfg WebCfg;
DHTesp dht;
unsigned long LastReading = 0;
int GpioDht = -1;

void setup() {
  Serial.begin(115200);

  //
  // Create an integer input.
  //
  WebCfg.setItem(0, "DHT11 GPIO", "DHT11_GPIO", ITEM_TYPE_INT);
  WebCfg.begin();

  GpioDht = WebCfg.getItemValueInt("DHT11_GPIO", -1);
  Serial.printf("DHT11 GPIO = %d\n", GpioDht);
  if (GpioDht >= 0) {
    // Initialize temperature sensor
    Serial.println("DHT initiated");
    dht.setup(GpioDht, DHTesp::DHT11);
  }
}

void loop() {
  WebCfg.handle();
  
  if (millis() - LastReading > 5000) { // Read the sensor every 5 seconds.
    if (GpioDht >= 0) {
      TempAndHumidity newValues = dht.getTempAndHumidity();
      Serial.printf("Temperature = %f\n", newValues.temperature);
      Serial.printf("Humidity = %f\n", newValues.humidity);
    }
    LastReading = millis();
  }
}
```
Connect to a WiFi SSID named FW-BOX_???? and open a web page [192.168.4.1](http://192.168.4.1 "192.168.4.1"). You can see a web page as below picture.
[![FwBox_WebCfg](https://github.com/fw-box/FwBox_WebCfg/blob/main/examples/Config_DHT11_GPIO/FwBox_WebCfg_web_page_Config_DHT11_GPIO.png?raw=true "FwBox_WebCfg")](https://github.com/fw-box/FwBox_WebCfg/blob/main/examples/Config_DHT11_GPIO/FwBox_WebCfg_web_page_Config_DHT11_GPIO.png?raw=true "FwBox_WebCfg")

# Example : String and Enable/Disable input
```cpp

#include "FwBox_WebCfg.h"
#include <PubSubClient.h>

FwBox_WebCfg WebCfg;
WiFiClient espClient;
PubSubClient MqttClient(espClient);
String MqttBrokerIp = "";

void setup() {
  Serial.begin(115200);

  //
  // Create two input in web page.
  //
  WebCfg.setItem(0, "MQTT Broker IP", "MQTT_IP"); // string input
  WebCfg.setItem(1, "MQTT Discovery", "MQTT_DISC", ITEM_TYPE_EN_DIS); // enable/disable select input
  WebCfg.begin();

  //
  // Get the value of "MQTT_IP" from web input.
  //
  MqttBrokerIp = WebCfg.getItemValueString("MQTT_IP");
  Serial.printf("MQTT Broker IP = %s\n", MqttBrokerIp.c_str());

  if (MqttBrokerIp.length() > 0) {
    MqttClient.setServer(MqttBrokerIp.c_str(), 1883);
    MqttClient.setCallback(callback);

    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String client_id = "Fw-Box-";
    client_id += WebCfg.getMac();
    // Attempt to connect
    if (MqttClient.connect(client_id.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(MqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      //delay(5000);
    }
  }
}

void loop() {
  WebCfg.handle();
  MqttClient.loop();
  //delay(2);
}

//
// Callback function for MQTT subscribe.
//
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
```
Connect to a WiFi SSID named FW-BOX_???? and open a web page [192.168.4.1](http://192.168.4.1 "192.168.4.1"). You can see a web page as below picture.
[![FwBox_WebCfg](https://github.com/fw-box/FwBox_WebCfg/blob/main/examples/Config_MqttBrokerIp/FwBox_WebCfg_web_page_Config_MqttBrokerIp.png?raw=true "FwBox_WebCfg")](https://github.com/fw-box/FwBox_WebCfg/blob/main/examples/Config_MqttBrokerIp/FwBox_WebCfg_web_page_Config_MqttBrokerIp.png?raw=true "FwBox_WebCfg")
