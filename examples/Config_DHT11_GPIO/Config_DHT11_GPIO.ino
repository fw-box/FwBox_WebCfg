//
// Copyright (c) 2022 Fw-Box (https://fw-box.com)
// Author: Hartman Hsieh
//
// Description :
//
// Libraries :
//   https://github.com/beegee-tokyo/DHTesp
//

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

  //
  // Get the GPIO of DHT from web config.
  //
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
