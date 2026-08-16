#pragma once

#include <Arduino.h>
#include "sensor_data.h"

namespace Telemetry {

void printBootBanner(Stream &out);

void printStatus(Stream &out,
                 bool imuReady, uint8_t imuAddress,
                 bool envReady, const char *envType, uint8_t envAddress);

void printHumanReadable(Stream &out,
                        uint32_t nowMs,
                        const ImuData &imu,
                        const EnvData &env);

void printJson(Stream &out,
               uint32_t nowMs,
               const ImuData &imu,
               const EnvData &env,
               const char *envType);

}  // namespace Telemetry