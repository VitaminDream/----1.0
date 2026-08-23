#include <Arduino.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>


#include "config.h"
#include "i2c_utils.h"
#include "sensor_data.h"
#include "sensors.h"
#include "oled_display.h"


// =====================================================
// 浮标样机 V1.3
// ESP32-S3
// MPU6050
// BME280
// GPS G60
// SD CARD
// OLED
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


bool sdOK=false;


// ================= Objects =============

BuoySensors sensors;

HardwareSerial G60Serial(1);

TinyGPSPlus gps;


// ================= Time ================

uint32_t lastSampleMs=0;



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



if(!SD.begin(SD_CS_PIN))
{

Serial.println("[SD] Card mount failed");

sdOK=false;

return;

}


sdOK=true;


Serial.println("[SD] Card OK");



File file=SD.open(
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

}

file.close();

}


}



// =====================================================
// GPS
// =====================================================

void serviceGps()
{

while(G60Serial.available())
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
EnvData &env
)
{


if(!sdOK)
return;



File file=SD.open(
"/buoy.csv",
FILE_APPEND
);



if(!file)
return;



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

file.println(imu.pitchDeg);



file.close();


}



// =====================================================
// 串口显示
// =====================================================

void printSerial(
ImuData &imu,
EnvData &env
)
{


Serial.println();

Serial.println(
"============ BUOY DATA ============"
);



Serial.printf(
"IMU Roll %.2f Pitch %.2f\n",
imu.rollDeg,
imu.pitchDeg
);


Serial.printf(
"ACC %.2f %.2f %.2f\n",
imu.ax,
imu.ay,
imu.az
);


Serial.printf(
"ENV Temp %.2f C\n",
env.temperatureC
);


Serial.printf(
"Pressure %.2f hPa\n",
env.pressureHpa
);



Serial.printf(
"Humidity %.1f %%\n",
env.humidityPct
);



if(gps.location.isValid())
{

Serial.println("GPS FIX");

}

else
{

Serial.println("GPS SEARCHING");

}


if(sdOK)
Serial.println("SD OK");
else
Serial.println("SD ERROR");



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
"BUOY V1.3 START"
);




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



/*
 OLED启动
*/
oledBegin();



/*
 SD
*/
initSD();



/*
 GPS
*/
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


serviceGps();



uint32_t now=millis();



if(
now-lastSampleMs
>=
BuoyConfig::SAMPLE_INTERVAL_MS
)
{


lastSampleMs=now;



ImuData imu;

EnvData env;



sensors.readImu(
imu
);


sensors.readEnvironment(
env
);




// 串口
printSerial(
imu,
env
);



// SD

saveData(
imu,
env
);



// OLED

oledUpdate(
imu,
env,
gps,
sdOK
);



}



delay(10);


}