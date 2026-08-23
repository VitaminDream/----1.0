#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H


#include <Arduino.h>
#include <TinyGPS++.h>

#include "sensor_data.h"


// OLED初始化
void oledBegin();


// OLED刷新显示
void oledUpdate(
    const ImuData &imu,
    const EnvData &env,
    TinyGPSPlus &gps,
    bool sdOK
);


#endif