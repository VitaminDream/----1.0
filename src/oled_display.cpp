#include "oled_display.h"

#include <Wire.h>
#include <U8g2lib.h>


// ============================================================
// 2.42寸 OLED
// SSD1309
// 128 x 64
// I2C
//
// SDA → GPIO8
// SCL → GPIO9
// 地址 → 0x3C
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
// OLED 初始化
// ============================================================

void oledBegin()
{
    Serial.println("[OLED] 正在初始化...");


    oled.setI2CAddress(
        0x3C << 1
    );


    oled.begin();


    // 中文字体
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


    Serial.println("[OLED] 初始化完成");
}


// ============================================================
// OLED 实时显示
// ============================================================

void oledUpdate(
    const ImuData &imu,
    const EnvData &env,
    TinyGPSPlus &gps,
    bool sdOK,
    float waterTemp
)
{
    if (!oledReady)
    {
        return;
    }


    oled.clearBuffer();


    oled.setFont(
        u8g2_font_wqy12_t_gb2312
    );


    char text[64];


    // ========================================================
    // 第1行
    // 环境温度 + 气压
    // ========================================================

    if (env.valid)
    {
        snprintf(
            text,
            sizeof(text),
            "环境 温%.1fC 气%.0f",
            env.temperatureC,
            env.pressureHpa
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "环境 数据异常"
        );
    }


    oled.drawUTF8(
        0,
        11,
        text
    );


    // ========================================================
    // 第2行
    // 湿度 + DS18B20水温
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
                "湿度%.0f%% 水温%.1fC",
                env.humidityPct,
                waterTemp
            );
        }
        else
        {
            snprintf(
                text,
                sizeof(text),
                "湿度%.0f%% 水温异常",
                env.humidityPct
            );
        }
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "湿度异常"
        );
    }


    oled.drawUTF8(
        0,
        24,
        text
    );


    // ========================================================
    // 第3行
    // 横滚 + 俯仰
    // ========================================================

    if (imu.valid)
    {
        snprintf(
            text,
            sizeof(text),
            "姿态 横%+.1f 俯%+.1f",
            imu.rollDeg,
            imu.pitchDeg
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "姿态 数据异常"
        );
    }


    oled.drawUTF8(
        0,
        37,
        text
    );


    // ========================================================
    // 第4行
    // XYZ加速度
    // 为了128像素宽度，只显示1位小数
    // ========================================================

    if (imu.valid)
    {
        snprintf(
            text,
            sizeof(text),
            "加速 X%.1f Y%.1f Z%.1f",
            imu.ax,
            imu.ay,
            imu.az
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "加速 数据异常"
        );
    }


    oled.drawUTF8(
        0,
        50,
        text
    );


    // ========================================================
    // 第5行
    // GPS + SD
    // ========================================================

    if (
        gps.location.isValid() &&
        gps.location.age() < 5000
    )
    {
        if (sdOK)
        {
            snprintf(
                text,
                sizeof(text),
                "定位 已定位 存储正常"
            );
        }
        else
        {
            snprintf(
                text,
                sizeof(text),
                "定位 已定位 存储异常"
            );
        }
    }
    else if (
        gps.charsProcessed() > 10
    )
    {
        if (sdOK)
        {
            snprintf(
                text,
                sizeof(text),
                "定位 搜星中 存储正常"
            );
        }
        else
        {
            snprintf(
                text,
                sizeof(text),
                "定位 搜星中 存储异常"
            );
        }
    }
    else
    {
        if (sdOK)
        {
            snprintf(
                text,
                sizeof(text),
                "定位 无数据 存储正常"
            );
        }
        else
        {
            snprintf(
                text,
                sizeof(text),
                "定位 无数据 存储异常"
            );
        }
    }


    oled.drawUTF8(
        0,
        63,
        text
    );


    oled.sendBuffer();
}