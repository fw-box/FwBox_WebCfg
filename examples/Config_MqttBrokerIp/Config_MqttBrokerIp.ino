
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
