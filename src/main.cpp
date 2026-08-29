#include <Arduino.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>


#include "config.h"
#include "i2c_utils.h"
#include "sensor_data.h"
#include "sensors.h"
#include "ds18b20.h"
#include "oled_display.h"


// =====================================================
// 浮标样机 V1.4
// ESP32-S3
// MPU6050
// BME280
// GPS G60
// SD CARD
// OLED
// DS18B20 WATER TEMPERATURE
// =====================================================



// ================= GPS =================

constexpr int GPS_RX_PIN = 18;
constexpr int GPS_TX_PIN = 17;

constexpr uint32_t GPS_BAUD = 9600;


// ================= SD ==================

constexpr int SD_CS_PIN = 10;
constexpr int SD_MOSI_PIN = 11;
constexpr int SD_MISO_PIN = 13;
constexpr int SD_SCK_PIN = 12;


bool sdOK = false;


// ================= Objects =============

BuoySensors sensors;

// DS18B20 防水水温传感器
// DATA → GPIO4
DS18B20 waterSensor;


HardwareSerial G60Serial(1);

TinyGPSPlus gps;


// ================= Time ================

uint32_t lastSampleMs = 0;



// =====================================================
// 判断 DS18B20 水温是否有效
// =====================================================

bool isWaterTempValid(float temperature)
{
    return (
        isfinite(temperature) &&
        temperature > -55.0f &&
        temperature <= 125.0f
    );
}



// =====================================================
// SD初始化
// =====================================================

void initSD()
{

    Serial.println("[SD] Initializing...");


    SPI.begin(
        SD_SCK_PIN,
        SD_MISO_PIN,
        SD_MOSI_PIN,
        SD_CS_PIN
    );


    if (!SD.begin(SD_CS_PIN))
    {

        Serial.println("[SD] Card mount failed");

        sdOK = false;

        return;
    }


    sdOK = true;


    Serial.println("[SD] Card OK");


    File file = SD.open(
        "/buoy.csv",
        FILE_APPEND
    );


    if (file)
    {

        if (file.size() == 0)
        {

            file.println(
                "time,lat,lon,temp,pressure,humidity,water_temp,roll,pitch"
            );

        }

        file.close();

    }

}



// =====================================================
// GPS
// =====================================================

void serviceGps()
{

    while (G60Serial.available())
    {

        gps.encode(
            G60Serial.read()
        );

    }

}



// =====================================================
// SD保存
// =====================================================

void saveData(
    ImuData &imu,
    EnvData &env,
    float waterTemp
)
{


    if (!sdOK)
        return;


    File file = SD.open(
        "/buoy.csv",
        FILE_APPEND
    );


    if (!file)
        return;



    // ================= 时间 =================

    file.print(millis());
    file.print(",");



    // ================= GPS =================

    if (gps.location.isValid())
    {

        file.print(
            gps.location.lat(),
            6
        );

        file.print(",");


        file.print(
            gps.location.lng(),
            6
        );

    }

    else
    {

        file.print("0,0");

    }



    // ================= BME280 温度 =================

    file.print(",");

    file.print(
        env.temperatureC
    );



    // ================= BME280 气压 =================

    file.print(",");

    file.print(
        env.pressureHpa
    );



    // ================= BME280 湿度 =================

    file.print(",");

    file.print(
        env.humidityPct
    );



    // ================= DS18B20 水温 =================

    file.print(",");

    if (isWaterTempValid(waterTemp))
    {

        file.print(
            waterTemp
        );

    }
    else
    {

        // 传感器异常时，该字段留空
        file.print("");

    }



    // ================= MPU6050 横滚 =================

    file.print(",");

    file.print(
        imu.rollDeg
    );



    // ================= MPU6050 俯仰 =================

    file.print(",");

    file.println(
        imu.pitchDeg
    );



    file.close();

}



// =====================================================
// 串口显示
// =====================================================

void printSerial(
    const ImuData &imu,
    const EnvData &env,
    float waterTemp
)
{

    Serial.println();

    Serial.println(
        "========== 浮标实时数据 =========="
    );


    // =================================================
    // 环境数据
    // =================================================

    if (env.valid)
    {

        Serial.printf(
            "环境   温度 %.2f C | 气压 %.2f hPa",
            env.temperatureC,
            env.pressureHpa
        );


        if (
            env.hasHumidity &&
            isfinite(env.humidityPct)
        )
        {

            Serial.printf(
                " | 湿度 %.1f %%",
                env.humidityPct
            );

        }

        Serial.println();

    }

    else
    {

        Serial.println(
            "环境   数据异常"
        );

    }



    // =================================================
    // DS18B20 水温
    // =================================================

    if (isWaterTempValid(waterTemp))
    {

        Serial.printf(
            "水温   %.2f C\n",
            waterTemp
        );

    }

    else
    {

        Serial.println(
            "水温   DS18B20传感器异常"
        );

    }



    // =================================================
    // 姿态数据
    // =================================================

    if (imu.valid)
    {

        Serial.printf(
            "姿态   横滚 %+6.2f | 俯仰 %+6.2f\n",
            imu.rollDeg,
            imu.pitchDeg
        );


        Serial.printf(
            "加速度 X %+5.2f | Y %+5.2f | Z %+5.2f m/s2\n",
            imu.ax,
            imu.ay,
            imu.az
        );

    }

    else
    {

        Serial.println(
            "姿态   数据异常"
        );

        Serial.println(
            "加速度 数据异常"
        );

    }



    // =================================================
    // GPS
    // =================================================

    Serial.print(
        "定位   "
    );


    if (
        gps.location.isValid() &&
        gps.location.age() < 5000
    )
    {

        Serial.printf(
            "已定位 | %.6f, %.6f",
            gps.location.lat(),
            gps.location.lng()
        );


        if (
            gps.satellites.isValid()
        )
        {

            Serial.printf(
                " | 卫星 %lu颗",
                gps.satellites.value()
            );

        }

        Serial.println();

    }

    else if (
        gps.charsProcessed() > 10
    )
    {

        Serial.println(
            "搜索卫星中..."
        );

    }

    else
    {

        Serial.println(
            "暂无GPS数据"
        );

    }



    // =================================================
    // SD卡
    // =================================================

    Serial.print(
        "存储   "
    );


    if (sdOK)
    {

        Serial.println(
            "SD卡正常"
        );

    }

    else
    {

        Serial.println(
            "SD卡异常"
        );

    }



    Serial.println(
        "================================="
    );

}



// =====================================================
// SETUP
// =====================================================

void setup()
{


    Serial.begin(
        BuoyConfig::SERIAL_BAUD
    );


    delay(1000);



    Serial.println(
        "BUOY V1.4 START"
    );



    // =================================================
    // I2C
    // MPU6050 + BME280 + OLED
    // =================================================

    Wire.begin(
        BuoyConfig::I2C_SDA_PIN,
        BuoyConfig::I2C_SCL_PIN,
        BuoyConfig::I2C_FREQUENCY_HZ
    );


    I2CUtils::scanBus(
        Wire,
        Serial
    );



    // =================================================
    // MPU6050 + BME280
    // 原有代码保持不变
    // =================================================

    sensors.begin(
        Wire,
        Serial
    );



    // =================================================
    // DS18B20
    // DATA → GPIO4
    // =================================================

    waterSensor.begin();


    Serial.println(
        "[DS18B20] Water temperature sensor started"
    );



    // =================================================
    // OLED启动
    // =================================================

    oledBegin();



    // =================================================
    // SD
    // =================================================

    initSD();



    // =================================================
    // GPS
    // =================================================

    G60Serial.begin(
        GPS_BAUD,
        SERIAL_8N1,
        GPS_RX_PIN,
        GPS_TX_PIN
    );


    Serial.println(
        "[G60] UART started"
    );



}



// =====================================================
// LOOP
// =====================================================

void loop()
{


    // GPS持续读取
    serviceGps();



    uint32_t now = millis();



    if (
        now - lastSampleMs
        >=
        BuoyConfig::SAMPLE_INTERVAL_MS
    )
    {


        lastSampleMs = now;



        ImuData imu;

        EnvData env;



        // =================================================
        // 原有 MPU6050
        // =================================================

        sensors.readImu(
            imu
        );



        // =================================================
        // 原有 BME280
        // =================================================

        sensors.readEnvironment(
            env
        );



        // =================================================
        // 新增 DS18B20 水温
        // =================================================

        float waterTemp =
            waterSensor.getTemperature();



        // =================================================
        // 串口
        // =================================================

        printSerial(
            imu,
            env,
            waterTemp
        );



        // =================================================
        // SD
        // =================================================

        saveData(
            imu,
            env,
            waterTemp
        );



        // =================================================
        // OLED
        // =================================================

        oledUpdate(
            imu,
            env,
            gps,
            sdOK,
            waterTemp
        );



    }



    delay(10);

}