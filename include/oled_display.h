#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "sensor_data.h"

// OLED 初始化
void oledBegin();

// OLED 刷新显示
//
// controlMode:
//   AUTO / HIGH / NORMAL / CALM
//
// workState:
//   HIGH / NORMAL / CALM
//
// sampleIntervalSec:
//   1 / 60 / 600
void oledUpdate(
    const ImuData &imu,
    const EnvData &env,
    TinyGPSPlus &gps,
    bool sdOK,
    float waterTemp,
    const char *controlMode,
    const char *workState,
    uint32_t sampleIntervalSec
);

#endif
