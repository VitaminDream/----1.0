#pragma once
#include <Arduino.h>

namespace BuoyConfig {

// ===== Hardware: V1 wiring =====
constexpr int I2C_SDA_PIN = 8;
constexpr int I2C_SCL_PIN = 9;
constexpr uint32_t I2C_FREQUENCY_HZ = 100000;  // 100 kHz: beginner-friendly and robust

// ===== Serial =====
constexpr uint32_t SERIAL_BAUD = 115200;

// ===== Sampling =====
constexpr uint32_t SAMPLE_INTERVAL_MS = 1000;

// ===== Common I2C addresses =====
constexpr uint8_t MPU6050_ADDR_1 = 0x68;
constexpr uint8_t MPU6050_ADDR_2 = 0x69;
constexpr uint8_t ENV280_ADDR_1 = 0x76;
constexpr uint8_t ENV280_ADDR_2 = 0x77;

// ===== Sea-level pressure for altitude estimate =====
// This is only a rough estimate. Later versions can calibrate it from weather/local pressure.
constexpr float SEA_LEVEL_PRESSURE_HPA = 1013.25f;

// ===== Gravity =====
constexpr float STANDARD_GRAVITY = 9.80665f;

}  // namespace BuoyConfig
