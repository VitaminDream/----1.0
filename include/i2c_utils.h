#pragma once
#include <Arduino.h>
#include <Wire.h>

namespace I2CUtils {

bool addressResponds(TwoWire &wire, uint8_t address);
bool readRegister8(TwoWire &wire, uint8_t address, uint8_t reg, uint8_t &value);
uint8_t scanBus(TwoWire &wire, Stream &out);

}  // namespace I2CUtils
