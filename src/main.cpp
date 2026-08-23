#include <Arduino.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>

#include "config.h"
#include "i2c_utils.h"
#include "sensor_data.h"
#include "sensors.h"


// ============================================================
// 浮标样机 V1.2
// ESP32-S3 + MPU6050 + BME280 + WHEELTEC G60 + SD
// ============================================================


// ========================
// GPS配置
// ========================

constexpr int GPS_RX_PIN = 18;
constexpr int GPS_TX_PIN = 17;

constexpr uint32_t GPS_BAUD = 9600;


// ========================
// SD配置
// ========================

constexpr int SD_CS_PIN = 10;
constexpr int SD_MOSI_PIN = 11;
constexpr int SD_MISO_PIN = 13;
constexpr int SD_SCK_PIN = 12;


bool sdOK = false;


// ========================
// 对象
// ========================

BuoySensors sensors;

HardwareSerial G60Serial(1);

TinyGPSPlus gps;



uint32_t lastSampleMs = 0;



// ============================================================
// SD初始化
// ============================================================

void initSD()
{

    Serial.println();
    Serial.println("[SD] Initializing...");


    SPI.begin(
        SD_SCK_PIN,
        SD_MISO_PIN,
        SD_MOSI_PIN,
        SD_CS_PIN
    );


    if(!SD.begin(
        SD_CS_PIN,
        SPI,
        4000000
    ))
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


    if(file)
    {

        if(file.size()==0)
        {

            file.println(
            "time,lat,lon,temp,pressure,humidity,roll,pitch"
            );

            Serial.println("[SD] CSV header created");

        }


        Serial.print("[SD] File size: ");
        Serial.print(file.size());
        Serial.println(" bytes");


        file.close();

    }

}



// ============================================================
// GPS读取
// ============================================================

void serviceGps()
{

    while(G60Serial.available())
    {

        gps.encode(
            G60Serial.read()
        );

    }

}



// ============================================================
// IMU显示
// ============================================================

void printImu(
    const ImuData &imu
)
{

    Serial.printf(
        "IMU | Roll=%.2f Pitch=%.2f\n",
        imu.rollDeg,
        imu.pitchDeg
    );


    Serial.printf(
        "    | Acc %.2f %.2f %.2f m/s2\n",
        imu.ax,
        imu.ay,
        imu.az
    );


    Serial.printf(
        "    | Gyro %.2f %.2f %.2f rad/s\n",
        imu.gx,
        imu.gy,
        imu.gz
    );

}



// ============================================================
// 环境显示
// ============================================================

void printEnvironment(
    const EnvData &env
)
{

    Serial.printf(
    "ENV | Temp=%.2f C Pressure=%.2f hPa",
    env.temperatureC,
    env.pressureHpa
    );


    if(env.hasHumidity)
    {

        Serial.printf(
        " Humidity=%.1f%%",
        env.humidityPct
        );

    }


    Serial.println();


    Serial.printf(
    "    | Altitude %.2f m\n",
    env.altitudeM
    );

}



// ============================================================
// GPS显示
// ============================================================

void printGps()
{

    Serial.print("GPS | ");


    if(
    gps.location.isValid()
    )
    {

        Serial.println("FIX");


        Serial.printf(
        "    | Lat %.6f Lon %.6f\n",
        gps.location.lat(),
        gps.location.lng()
        );


        Serial.printf(
        "    | Satellites %d\n",
        gps.satellites.value()
        );

    }

    else if(
    gps.charsProcessed()>10
    )
    {

        Serial.println("SEARCHING");


        Serial.printf(
        "    | Characters=%lu\n",
        gps.charsProcessed()
        );


        Serial.println(
        "    | Waiting satellites..."
        );

    }

    else
    {

        Serial.println("NO DATA");

    }

}



// ============================================================
// 保存SD
// ============================================================

void saveData(
const ImuData &imu,
const EnvData &env
)
{

    if(!sdOK)
    {

        Serial.println("[SD] Not ready");

        return;

    }



    File file = SD.open(
        "/buoy.csv",
        FILE_APPEND
    );


    if(!file)
    {

        Serial.println("[SD] Open failed");

        return;

    }



    file.print(millis());
    file.print(",");



    if(gps.location.isValid())
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


    file.print(",");

    file.print(env.temperatureC);


    file.print(",");

    file.print(env.pressureHpa);


    file.print(",");

    file.print(env.humidityPct);


    file.print(",");

    file.print(imu.rollDeg);


    file.print(",");


    file.println(
    imu.pitchDeg
    );


    file.close();



    Serial.println("[SD] Data saved OK");

}



// ============================================================
// setup
// ============================================================

void setup()
{

    Serial.begin(
    BuoyConfig::SERIAL_BAUD
    );


    delay(1000);


    Serial.println();
    Serial.println("==============================");
    Serial.println(" Buoy Prototype V1.2");
    Serial.println(" MPU6050 + BME280 + G60 + SD");
    Serial.println("==============================");



    Wire.begin(
    BuoyConfig::I2C_SDA_PIN,
    BuoyConfig::I2C_SCL_PIN,
    BuoyConfig::I2C_FREQUENCY_HZ
    );



    I2CUtils::scanBus(
    Wire,
    Serial
    );



    sensors.begin(
    Wire,
    Serial
    );



    initSD();



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



// ============================================================
// loop
// ============================================================

void loop()
{

    serviceGps();



    uint32_t now = millis();



    if(
    now-lastSampleMs >=
    BuoyConfig::SAMPLE_INTERVAL_MS
    )
    {

        lastSampleMs = now;



        ImuData imu;

        EnvData env;



        sensors.readImu(imu);

        sensors.readEnvironment(env);



        Serial.println();

        Serial.println(
        "============= BUOY DATA ============="
        );



        printImu(imu);


        printEnvironment(env);


        printGps();



        saveData(
        imu,
        env
        );



        Serial.println(
        "====================================="
        );


    }



    delay(2);

}