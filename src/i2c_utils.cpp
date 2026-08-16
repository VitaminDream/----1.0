#include "i2c_utils.h"

namespace I2CUtils {

bool addressResponds(TwoWire &wire, uint8_t address) {
  wire.beginTransmission(address);
  return wire.endTransmission() == 0;
}

bool readRegister8(TwoWire &wire, uint8_t address, uint8_t reg, uint8_t &value) {
  wire.beginTransmission(address);
  wire.write(reg);
  if (wire.endTransmission(false) != 0) {
    return false;
  }

  if (wire.requestFrom(static_cast<int>(address), 1) != 1) {
    return false;
  }

  value = wire.read();
  return true;
}

uint8_t scanBus(TwoWire &wire, Stream &out) {
  uint8_t count = 0;
  out.println("[I2C] Scanning 0x08..0x77 ...");

  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    wire.beginTransmission(address);
    const uint8_t error = wire.endTransmission();
    if (error == 0) {
      out.printf("[I2C] Found device at 0x%02X\n", address);
      ++count;
    }
  }

  out.printf("[I2C] Scan complete: %u device(s) found.\n", count);
  return count;
}

}  // namespace I2CUtils
