# 🌊 Intelligent Marine Buoy Prototype

## 基于 ESP32-S3 的智能海洋环境感知浮标样机

## 📌 Project Overview

本项目是一款基于 ESP32-S3 微控制器开发的智能浮标原型系统，主要用于实现海洋环境数据采集、浮标姿态监测以及实时定位功能。

该样机通过多传感器融合方式，实现对浮标运行状态和周围环境参数的实时感知，为后续海洋风险评估、航行辅助决策以及智能导航应用提供基础数据支持。

当前版本为第一阶段样机（Prototype V1.0），主要完成了硬件平台搭建、多传感器接入以及基础数据采集功能。

目前已实现：

- ✅ ESP32-S3 主控平台搭建
- ✅ MPU6050 六轴姿态传感器接入
- ✅ BMP280/BME280 环境传感器接入
- ✅ GPS 定位模块接入
- ✅ I2C 多传感器通信
- ✅ UART GPS 数据通信
- ✅ VSCode + PlatformIO 开发环境部署
- ✅ 传感器数据串口实时输出


---

# 🛠 Hardware Architecture

## 1. Main Controller

### ESP32-S3 Development Board

作为系统核心控制单元，负责：

- 传感器数据采集
- 数据处理
- 模块通信
- 程序运行控制


---

# 2. Sensor Modules


## MPU6050 Six-Axis Motion Sensor

功能：

- 三轴加速度检测
- 三轴角速度检测
- 浮标姿态变化监测
- 晃动状态分析


接线：

| MPU6050 | ESP32-S3 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |


---

## BMP280 / BME280 Environmental Sensor

功能：

- 气压检测
- 温度检测
- 高度变化估算
- 环境状态监测


接线：

| BMP280/BME280 | ESP32-S3 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |
| CSB | 3.3V |
| SDO | GND |


说明：

BMP280/BME280 与 MPU6050 共用 ESP32-S3 的 I2C 通信总线。

I2C连接方式：

```
ESP32-S3 GPIO8  -------- SDA -------- MPU6050
                                   |
                                   |
                                   -------- BMP280/BME280


ESP32-S3 GPIO9  -------- SCL -------- MPU6050
                                   |
                                   |
                                   -------- BMP280/BME280
```


---

## GPS Position Module

功能：

- 获取实时经纬度
- 获取定位时间
- 提供浮标空间位置数据


接线：

| GPS Module | ESP32-S3 |
|---|---|
| VCC | 3.3V / 5V |
| GND | GND |
| TX | ESP32 RX |
| RX | ESP32 TX |


通信方式：

UART Serial Communication


---

# 🔌 Complete Wiring Diagram

整体系统连接：

```
                        ESP32-S3
                           |
        ---------------------------------------
        |                  |                  |
       I2C                UART               USB
        |                  |                  |
        |                  |                  |
   MPU6050              GPS Module           PC
        |
        |
   BMP280/BME280


I2C BUS:

GPIO8  ---------------- SDA
GPIO9  ---------------- SCL


POWER:

3.3V ---------------- VCC
GND  ---------------- GND

```


硬件连接结构：

```
              +----------------+
              |    ESP32-S3    |
              +----------------+
                |      |     |
                |      |     |
              I2C    UART   USB
                |      |
        --------       --------
        |                    |
   MPU6050              GPS Module
        |
        |
   BMP280/BME280

```


---

# 💻 Software Development Environment


开发环境：

- Visual Studio Code
- PlatformIO
- Arduino Framework


项目结构：

```
Marine_Buoy/
│
├── src/
│   └── main.cpp
│
├── include/
│
├── lib/
│
├── platformio.ini
│
└── README.md

```


---

# 📡 Current Functions


## Multi Sensor Data Acquisition

系统目前可以采集：

### Motion Data

- Acceleration X/Y/Z
- Gyroscope X/Y/Z


### Environmental Data

- Temperature
- Atmospheric Pressure
- Altitude Estimation


### Position Data

- Latitude
- Longitude
- GPS Time


---

# 📟 Serial Output Example


当前运行状态通过串口输出：

```
========== Marine Buoy Status ==========

Temperature:
25.6 ℃

Pressure:
1008 hPa

Altitude:
42.5 m


Motion:

Acceleration:
X:0.02
Y:-0.01
Z:9.81


GPS:

Latitude:
xxxx.xxxx

Longitude:
xxxx.xxxx


========================================
```


---

# 🚀 Development Roadmap


## Version 1.0 - Basic Sensor Platform

Completed:

- ESP32-S3 controller
- MPU6050 attitude sensing
- BMP280/BME280 environmental sensing
- GPS positioning
- Sensor communication framework


---

## Version 2.0 - Data Storage and Communication

Future improvements:

- OLED display
- SD card storage
- LoRa wireless communication
- Remote monitoring
- More marine sensors


---

## Version 3.0 - Intelligent Marine Decision System

Future goals:

- Multi-source environmental data fusion
- Marine risk assessment
- Dynamic route analysis
- Navigation assistance
- Intelligent buoy decision support


---

# 🎯 Project Goal

本项目目标是构建一个低成本、模块化、可扩展的智能海洋感知浮标平台。

通过 ESP32-S3 与多传感器融合技术，实现对海洋环境和浮标状态的实时监测，为未来无人船、智能航行以及海洋风险分析系统提供可靠的数据采集基础。


---

# 📦 Hardware List

| Component | Function |
|---|---|
| ESP32-S3 | Main Controller |
| MPU6050 | Motion Detection |
| BMP280/BME280 | Environmental Monitoring |
| GPS Module | Positioning |
| Breadboard | Prototype Connection |
| Dupont Wires | Hardware Connection |


---

# 👨‍💻 Development Information

Hardware Platform:

- ESP32-S3
- MPU6050
- BMP280/BME280
- GPS Module


Software Platform:

- VSCode
- PlatformIO
- Arduino Framework


Author:

VXDVV
