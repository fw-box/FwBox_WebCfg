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
Connect a WiFi SSID named FW-BOX_???? and open a web page [192.168.4.1](http://192.168.4.1 "192.168.4.1"). You can see a web page as below picture.
[![FwBox_WebCfg](https://github.com/fw-box/FwBox_WebCfg/blob/main/examples/Config_DHT11_GPIO/FwBox_WebCfg_web_page_Config_DHT11_GPIO.png?raw=true "FwBox_WebCfg")](https://github.com/fw-box/FwBox_WebCfg/blob/main/examples/Config_DHT11_GPIO/FwBox_WebCfg_web_page_Config_DHT11_GPIO.png?raw=true "FwBox_WebCfg")



