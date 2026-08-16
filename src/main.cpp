#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "i2c_utils.h"
#include "sensor_data.h"
#include "sensors.h"
#include "telemetry.h"

BuoySensors sensors;
uint32_t lastSampleMs = 0;

void setup() {
  Serial.begin(BuoyConfig::SERIAL_BAUD);
  delay(1200);  // Give USB serial time to enumerate after reset.

  Telemetry::printBootBanner(Serial);

  if (!Wire.begin(BuoyConfig::I2C_SDA_PIN,
                  BuoyConfig::I2C_SCL_PIN,
                  BuoyConfig::I2C_FREQUENCY_HZ)) {
    Serial.println("[FATAL] Wire.begin() failed. Check GPIO8/GPIO9 and board config.");
    return;
  }

  I2CUtils::scanBus(Wire, Serial);
  sensors.begin(Wire, Serial);

  Telemetry::printStatus(Serial,
                         sensors.imuReady(), sensors.imuAddress(),
                         sensors.envReady(), sensors.envTypeName(), sensors.envAddress());

  Serial.println("[READY] V1 sampling started. One sample per second.");
  Serial.println("[READY] Lines beginning with JSON: can be used by a future PC dashboard.");
}

void loop() {
  const uint32_t nowMs = millis();
  if (nowMs - lastSampleMs < BuoyConfig::SAMPLE_INTERVAL_MS) {
    delay(5);
    return;
  }
  lastSampleMs = nowMs;

  ImuData imu;
  EnvData env;

  sensors.readImu(imu);
  sensors.readEnvironment(env);

  Telemetry::printHumanReadable(Serial, nowMs, imu, env);
  
}