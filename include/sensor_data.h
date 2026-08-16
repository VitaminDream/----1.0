#pragma once
#include <Arduino.h>
#include <math.h>

struct ImuData {
  bool valid = false;
  float ax = NAN;
  float ay = NAN;
  float az = NAN;
  float gx = NAN;
  float gy = NAN;
  float gz = NAN;
  float temperatureC = NAN;
  float rollDeg = NAN;
  float pitchDeg = NAN;
  float accelMagnitude = NAN;
  float dynamicAccel = NAN;
};

struct EnvData {
  bool valid = false;
  bool hasHumidity = false;
  float temperatureC = NAN;
  float pressureHpa = NAN;
  float humidityPct = NAN;
  float altitudeM = NAN;
};

enum class EnvSensorType : uint8_t {
  NONE = 0,
  BMP280,
  BME280
};
