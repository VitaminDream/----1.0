#include "oled_display.h"

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>


// ============================================================
// 2.42寸 OLED
// SSD1309
// 128 x 64
// I2C
//
// SDA -> GPIO8
// SCL -> GPIO9
// 地址 -> 0x3C
// ============================================================

U8G2_SSD1309_128X64_NONAME0_F_HW_I2C oled(
    U8G2_R0,
    U8X8_PIN_NONE
);

bool oledReady = false;


// ============================================================
// 判断 DS18B20 水温是否有效
// ============================================================

bool oledWaterTempValid(float temperature)
{
    return (
        isfinite(temperature) &&
        temperature > -55.0f &&
        temperature <= 125.0f
    );
}


// ============================================================
// 获取 ESP32 当前北京时间
//
// 本版时间规则：
// main.cpp 在启动后把 ESP32 系统时钟固定设置为：
// 2026-08-31 20:50:00（北京时间）
//
// 之后由 ESP32 系统时钟自行连续走时。
// GPS 完全不参与时间，只负责定位。
// ============================================================

bool oledGetBeijingTime(
    char *buffer,
    size_t bufferSize
)
{
    time_t currentTime;
    time(&currentTime);

    struct tm localTime;

    if (
        localtime_r(
            &currentTime,
            &localTime
        ) == nullptr
    )
    {
        return false;
    }

    int year =
        localTime.tm_year + 1900;

    // 系统时钟未正确初始化时不显示错误日期
    if (
        year < 2024 ||
        year > 2099
    )
    {
        return false;
    }

    snprintf(
        buffer,
        bufferSize,
        "%04d-%02d-%02d %02d:%02d:%02d",
        year,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec
    );

    return true;
}


// ============================================================
// OLED 初始化
// ============================================================

void oledBegin()
{
    Serial.println(
        "[OLED] 正在初始化..."
    );

    oled.setI2CAddress(
        0x3C << 1
    );

    oled.begin();

    oled.setFont(
        u8g2_font_wqy12_t_gb2312
    );

    oled.clearBuffer();

    oled.drawUTF8(
        30,
        25,
        "浮标系统"
    );

    oled.drawUTF8(
        30,
        45,
        "启动完成"
    );

    oled.sendBuffer();

    oledReady = true;

    delay(1000);

    Serial.println(
        "[OLED] 初始化完成"
    );
}


// ============================================================
// OLED 实时显示
//
// 128 x 64 一页显示：
//
// 第1行：固定起始时间后连续走时
// 第2行：环境温度 + 气压
// 第3行：湿度 + 水温
// 第4行：横滚 + 俯仰
// 第5行：控制模式 > 实际状态 + 采样周期
// 第6行：GPS状态 + SD状态
//
// 示例：
// 2026-08-31 20:50:18
// 温28.6 气998
// 湿41% 水温28.4
// 横-0.8 俯+2.9
// AUTO>NORMAL 60s
// GPS搜星 SD正常
//
// 注意：
// AUTO 下 MPU6050 仍由 main.cpp 每秒轻量监测。
// OLED这里只显示最近一次完整采样结果。
// ============================================================

void oledUpdate(
    const ImuData &imu,
    const EnvData &env,
    TinyGPSPlus &gps,
    bool sdOK,
    float waterTemp,
    const char *controlMode,
    const char *workState,
    uint32_t sampleIntervalSec
)
{
    if (!oledReady)
    {
        return;
    }

    oled.clearBuffer();

    char text[64];


    // ========================================================
    // 第1行：时间
    // ========================================================

    oled.setFont(
        u8g2_font_6x10_tf
    );

    if (
        !oledGetBeijingTime(
            text,
            sizeof(text)
        )
    )
    {
        snprintf(
            text,
            sizeof(text),
            "---- -- -- --:--:--"
        );
    }

    oled.drawStr(
        0,
        9,
        text
    );


    // ========================================================
    // 第2~6行：中文信息
    // ========================================================

    oled.setFont(
        u8g2_font_wqy12_t_gb2312
    );


    // ========================================================
    // 第2行：环境温度 + 气压
    // ========================================================

    if (env.valid)
    {
        snprintf(
            text,
            sizeof(text),
            "温%.1f 气%.0f",
            env.temperatureC,
            env.pressureHpa
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "温异常 气异常"
        );
    }

    oled.drawUTF8(
        0,
        20,
        text
    );


    // ========================================================
    // 第3行：湿度 + 水温
    // ========================================================

    if (
        env.valid &&
        env.hasHumidity &&
        isfinite(env.humidityPct)
    )
    {
        if (
            oledWaterTempValid(
                waterTemp
            )
        )
        {
            snprintf(
                text,
                sizeof(text),
                "湿%.0f%% 水温%.1f",
                env.humidityPct,
                waterTemp
            );
        }
        else
        {
            snprintf(
                text,
                sizeof(text),
                "湿%.0f%% 水温异常",
                env.humidityPct
            );
        }
    }
    else
    {
        if (
            oledWaterTempValid(
                waterTemp
            )
        )
        {
            snprintf(
                text,
                sizeof(text),
                "湿异常 水温%.1f",
                waterTemp
            );
        }
        else
        {
            snprintf(
                text,
                sizeof(text),
                "湿异常 水温异常"
            );
        }
    }

    oled.drawUTF8(
        0,
        31,
        text
    );


    // ========================================================
    // 第4行：Roll + Pitch
    // ========================================================

    if (imu.valid)
    {
        snprintf(
            text,
            sizeof(text),
            "横%+.1f 俯%+.1f",
            imu.rollDeg,
            imu.pitchDeg
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "横异常 俯异常"
        );
    }

    oled.drawUTF8(
        0,
        42,
        text
    );


    // ========================================================
    // 第5行：模式
    //
    // AUTO>NORMAL 60s
    // AUTO>HIGH 1s
    // AUTO>CALM 600s
    //
    // 手动模式：
    // HIGH>HIGH 1s
    // NORMAL>NORMAL 60s
    // CALM>CALM 600s
    // ========================================================

    oled.setFont(
        u8g2_font_6x10_tf
    );

    snprintf(
        text,
        sizeof(text),
        "%s>%s %lus",
        controlMode,
        workState,
        static_cast<unsigned long>(
            sampleIntervalSec
        )
    );

    oled.drawStr(
        0,
        52,
        text
    );


    // ========================================================
    // 第6行：GPS + SD
    // ========================================================

    oled.setFont(
        u8g2_font_wqy12_t_gb2312
    );

    if (
        gps.location.isValid() &&
        gps.location.age() < 5000
    )
    {
        snprintf(
            text,
            sizeof(text),
            sdOK
                ? "GPS已定位 SD正常"
                : "GPS已定位 SD异常"
        );
    }
    else if (
        gps.charsProcessed() > 10
    )
    {
        snprintf(
            text,
            sizeof(text),
            sdOK
                ? "GPS搜星 SD正常"
                : "GPS搜星 SD异常"
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            sdOK
                ? "GPS无数据 SD正常"
                : "GPS无数据 SD异常"
        );
    }

    oled.drawUTF8(
        0,
        63,
        text
    );


    // ========================================================
    // 刷新 OLED
    // ========================================================

    oled.sendBuffer();
}
