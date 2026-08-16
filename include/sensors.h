#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include "sensor_data.h"

class BuoySensors {
 public:
  bool begin(TwoWire &wire, Stream &out);
  bool readImu(ImuData &data);
  bool readEnvironment(EnvData &data);

  bool imuReady() const { return imuReady_; }
  bool envReady() const { return envReady_; }
  uint8_t imuAddress() const { return imuAddress_; }
  uint8_t envAddress() const { return envAddress_; }
  EnvSensorType envType() const { return envType_; }
  const char *envTypeName() const;

 private:
  bool beginImu(TwoWire &wire, Stream &out);
  bool beginEnv(TwoWire &wire, Stream &out);

  Adafruit_MPU6050 mpu_;
  Adafruit_BME280 bme_;
  Adafruit_BMP280 bmp_{&Wire};

  bool imuReady_ = false;
  bool envReady_ = false;
  uint8_t imuAddress_ = 0;
  uint8_t envAddress_ = 0;
  EnvSensorType envType_ = EnvSensorType::NONE;
};
