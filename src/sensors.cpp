#include "sensors.h"
#include <math.h>
#include "config.h"
#include "i2c_utils.h"

namespace {
constexpr uint8_t REG_CHIP_ID_280 = 0xD0;
constexpr uint8_t CHIP_ID_BMP280 = 0x58;
constexpr uint8_t CHIP_ID_BME280 = 0x60;
constexpr float RAD_TO_DEG_F = 57.2957795131f;
}

bool BuoySensors::begin(TwoWire &wire, Stream &out) {
  imuReady_ = beginImu(wire, out);
  envReady_ = beginEnv(wire, out);
  return imuReady_ || envReady_;
}

bool BuoySensors::beginImu(TwoWire &wire, Stream &out) {
  const uint8_t candidates[] = {
      BuoyConfig::MPU6050_ADDR_1,
      BuoyConfig::MPU6050_ADDR_2,
  };

  for (uint8_t address : candidates) {
    if (!I2CUtils::addressResponds(wire, address)) {
      continue;
    }

    if (mpu_.begin(address, &wire)) {
      imuAddress_ = address;
      mpu_.setAccelerometerRange(MPU6050_RANGE_4_G);
      mpu_.setGyroRange(MPU6050_RANGE_500_DEG);
      mpu_.setFilterBandwidth(MPU6050_BAND_21_HZ);
      out.printf("[OK] MPU6050 ready at 0x%02X\n", imuAddress_);
      return true;
    }
  }

  out.println("[WARN] MPU6050 not detected at 0x68/0x69.");
  return false;
}

bool BuoySensors::beginEnv(TwoWire &wire, Stream &out) {
  const uint8_t candidates[] = {
      BuoyConfig::ENV280_ADDR_1,
      BuoyConfig::ENV280_ADDR_2,
  };

  for (uint8_t address : candidates) {
    if (!I2CUtils::addressResponds(wire, address)) {
      continue;
    }

    uint8_t chipId = 0;
    if (!I2CUtils::readRegister8(wire, address, REG_CHIP_ID_280, chipId)) {
      continue;
    }

    if (chipId == CHIP_ID_BME280) {
      if (bme_.begin(address, &wire)) {
        envType_ = EnvSensorType::BME280;
        envAddress_ = address;
        bme_.setSampling(Adafruit_BME280::MODE_NORMAL,
                         Adafruit_BME280::SAMPLING_X2,
                         Adafruit_BME280::SAMPLING_X16,
                         Adafruit_BME280::SAMPLING_X1,
                         Adafruit_BME280::FILTER_X4,
                         Adafruit_BME280::STANDBY_MS_500);
        out.printf("[OK] BME280 ready at 0x%02X (chip ID 0x%02X)\n",
                   envAddress_, chipId);
        return true;
      }
    } else if (chipId == CHIP_ID_BMP280) {
      // The BMP object was created on Wire; begin() selects the address.
      if (bmp_.begin(address)) {
        envType_ = EnvSensorType::BMP280;
        envAddress_ = address;
        bmp_.setSampling(Adafruit_BMP280::MODE_NORMAL,
                         Adafruit_BMP280::SAMPLING_X2,
                         Adafruit_BMP280::SAMPLING_X16,
                         Adafruit_BMP280::FILTER_X4,
                         Adafruit_BMP280::STANDBY_MS_500);
        out.printf("[OK] BMP280 ready at 0x%02X (chip ID 0x%02X)\n",
                   envAddress_, chipId);
        return true;
      }
    } else {
      out.printf("[WARN] Device at 0x%02X has unexpected 280 chip ID 0x%02X.\n",
                 address, chipId);
    }
  }

  out.println("[WARN] No BME280/BMP280 detected at 0x76/0x77.");
  return false;
}

bool BuoySensors::readImu(ImuData &data) {
  data = ImuData{};
  if (!imuReady_) {
    return false;
  }

  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  if (!mpu_.getEvent(&accel, &gyro, &temp)) {
    return false;
  }

  data.ax = accel.acceleration.x;
  data.ay = accel.acceleration.y;
  data.az = accel.acceleration.z;
  data.gx = gyro.gyro.x;
  data.gy = gyro.gyro.y;
  data.gz = gyro.gyro.z;
  data.temperatureC = temp.temperature;

  data.accelMagnitude = sqrtf(data.ax * data.ax + data.ay * data.ay + data.az * data.az);
  data.dynamicAccel = fabsf(data.accelMagnitude - BuoyConfig::STANDARD_GRAVITY);

  // First-version tilt estimate from gravity vector.
  // It is useful when the buoy is moving slowly, but it is NOT full AHRS orientation.
  data.rollDeg = atan2f(data.ay, data.az) * RAD_TO_DEG_F;
  data.pitchDeg = atan2f(-data.ax, sqrtf(data.ay * data.ay + data.az * data.az)) * RAD_TO_DEG_F;

  data.valid = true;
  return true;
}

bool BuoySensors::readEnvironment(EnvData &data) {
  data = EnvData{};
  if (!envReady_) {
    return false;
  }

  if (envType_ == EnvSensorType::BME280) {
    data.temperatureC = bme_.readTemperature();
    data.pressureHpa = bme_.readPressure() / 100.0f;
    data.humidityPct = bme_.readHumidity();
    data.altitudeM = bme_.readAltitude(BuoyConfig::SEA_LEVEL_PRESSURE_HPA);
    data.hasHumidity = true;
  } else if (envType_ == EnvSensorType::BMP280) {
    data.temperatureC = bmp_.readTemperature();
    data.pressureHpa = bmp_.readPressure() / 100.0f;
    data.altitudeM = bmp_.readAltitude(BuoyConfig::SEA_LEVEL_PRESSURE_HPA);
    data.humidityPct = NAN;
    data.hasHumidity = false;
  } else {
    return false;
  }

  data.valid = isfinite(data.temperatureC) && isfinite(data.pressureHpa);
  return data.valid;
}

const char *BuoySensors::envTypeName() const {
  switch (envType_) {
    case EnvSensorType::BME280:
      return "BME280";
    case EnvSensorType::BMP280:
      return "BMP280";
    default:
      return "NONE";
  }
}