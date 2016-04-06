#include "common.h"
#include "node_mqtt.h"
#include "node_soilmoisture.h"
#include <Arduino.h>
#include "WiFiClient.h"
#include "DebugSerial.h"

void NodeSoilMoisture::setup(void)
{
  m_loop_only_if_connected = true;

  m_ads1115.begin();
  m_ads1115.set_comp_queue(ADS1115_COMP_QUEUE_DISABLE);
  m_ads1115.set_mode(ADS1115_MODE_SINGLE_SHOT);
  m_ads1115.set_pga(ADS1115_PGA_ONE);
}

float NodeSoilMoisture::read_value(enum ads1115_mux mux)
{
  const int NUM_WAITS = 10;
  m_ads1115.set_mux(mux);

  m_ads1115.trigger_sample();

  int i;
  for (i = 0; i < NUM_WAITS; i++)
  {
    delay(3);
    if (m_ads1115.is_sample_in_progress())
      break;
  }
  if (i == NUM_WAITS)
  {
    Serial.println("read timedout");
    return 0xFFFF;
  }
  return m_ads1115.read_sample_float();
}

unsigned NodeSoilMoisture::loop(void)
{
  // We read the battery through a 2:1 voltage divider so upscale it in the ADC
  m_ads1115.set_pga(ADS1115_PGA_TWO);
  float bat = read_value(ADS1115_MUX_GND_AIN0)*2.0;
  mqtt_publish_float("battery", bat);

  // The other values are read without any filtering and have a max of 3.3v so no scaling
  m_ads1115.set_pga(ADS1115_PGA_ONE);

  float moisture_analog = read_value(ADS1115_MUX_GND_AIN2);
  mqtt_publish_float("moisture", moisture_analog);

  float moisture_digital = read_value(ADS1115_MUX_GND_AIN3);
  mqtt_publish_float("trigger", moisture_digital);

  Serial.print("Time:");
  Serial.print(millis());
  Serial.print(" Battery: ");
  Serial.print(bat, 4);
  Serial.print(" Moisture: ");
  Serial.print(moisture_analog, 4);
  Serial.print(" Digital: ");
  Serial.println(moisture_digital, 4);

  return DEFAULT_DEEP_SLEEP_TIME;
}
