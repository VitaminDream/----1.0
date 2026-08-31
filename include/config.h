#pragma once

#include <Arduino.h>


namespace BuoyConfig
{

// ============================================================
// Hardware: V1 wiring
// ============================================================

// I2C
// MPU6050 + BME280 + OLED 共用

constexpr int I2C_SDA_PIN = 8;

constexpr int I2C_SCL_PIN = 9;

constexpr uint32_t I2C_FREQUENCY_HZ = 100000;


// ============================================================
// Serial
// ============================================================

constexpr uint32_t SERIAL_BAUD = 115200;


// ============================================================
// Sampling
// ============================================================

// 传感器统一每 1 分钟采集一次
//
// 60000 ms
// = 60 s
// = 1 min

constexpr uint32_t SAMPLE_INTERVAL_MS = 60000;


// ============================================================
// Common I2C addresses
// ============================================================

// MPU6050

constexpr uint8_t MPU6050_ADDR_1 = 0x68;

constexpr uint8_t MPU6050_ADDR_2 = 0x69;


// BME280 / BMP280

constexpr uint8_t ENV280_ADDR_1 = 0x76;

constexpr uint8_t ENV280_ADDR_2 = 0x77;


// ============================================================
// Sea-level pressure
// ============================================================

// 用于粗略海拔估算
// 后续可以根据实际当地气压进行校准

constexpr float SEA_LEVEL_PRESSURE_HPA = 1013.25f;


// ============================================================
// Gravity
// ============================================================

constexpr float STANDARD_GRAVITY = 9.80665f;


}  // namespace BuoyConfig