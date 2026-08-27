# 🌊 Intelligent Marine Buoy Prototype

## 基于 ESP32-S3 的智能海洋环境感知浮标样机

## 📌 Project Overview

本项目是一款基于 ESP32-S3 微控制器开发的智能浮标原型系统，主要用于实现海洋环境数据采集、浮标姿态监测、实时定位、本地数据显示以及独立能源供电功能。

该样机通过多传感器融合方式，实现对浮标运行状态和周围环境参数的实时感知，为后续海洋风险评估、航行辅助决策以及智能导航应用提供基础数据支持。

当前样机已在基础传感器平台上完成 GPS 定位、OLED 显示以及太阳能能源系统扩展，形成了较完整的 **“环境感知 + 姿态监测 + 位置定位 + 本地显示 + 独立供电”** 基础功能链路。

目前已实现：

* ✅ ESP32-S3 主控平台搭建
* ✅ MPU6050 六轴姿态传感器接入
* ✅ BMP280/BME280 环境传感器接入
* ✅ GPS 定位模块接入
* ✅ OLED 显示模块接入
* ✅ I2C 多传感器通信
* ✅ UART GPS 数据通信
* ✅ OLED 本地实时数据显示
* ✅ 太阳能板接入
* ✅ 太阳能电源管理模块接入
* ✅ 锂电池储能与供电链路接入
* ✅ 太阳能能源系统基础供电链路打通
* ✅ VSCode + PlatformIO 开发环境部署
* ✅ 传感器数据串口实时输出
* ✅ 基础功能模块化程序设计

> **Current Update**
>
> 当前太阳能板、太阳能电源管理模块、锂电池与 ESP32-S3 开发板之间的能源链路已经完成基础连接与供电测试。
>
> OLED 显示模块后续计划升级为 **2.42 英寸显示屏**，当前 I2C 接线及 GPIO8 / GPIO9 引脚方案暂时保留，待新屏幕接入后再根据实际驱动型号进行软件适配。
>
> **DS18B20 防水温度传感器目前尚未完成接入，作为下一阶段待更新模块。**

---

# 🛠 Hardware Architecture

## 1. Main Controller

### ESP32-S3 Development Board

作为系统核心控制单元，负责：

* 传感器数据采集
* GPS 数据接收
* 数据处理
* OLED 显示控制
* 模块通信
* 程序运行控制

---

# 2. Sensor Modules

## MPU6050 Six-Axis Motion Sensor

功能：

* 三轴加速度检测
* 三轴角速度检测
* 浮标姿态变化监测
* 晃动状态分析

接线：

| MPU6050 | ESP32-S3 |
| ------- | -------- |
| VCC     | 3.3V     |
| GND     | GND      |
| SDA     | GPIO8    |
| SCL     | GPIO9    |

---

## BMP280 / BME280 Environmental Sensor

功能：

* 气压检测
* 温度检测
* 高度变化估算
* 环境状态监测

接线：

| BMP280/BME280 | ESP32-S3 |
| ------------- | -------- |
| VCC           | 3.3V     |
| GND           | GND      |
| SDA           | GPIO8    |
| SCL           | GPIO9    |
| CSB           | 3.3V     |
| SDO           | GND      |

说明：

BMP280/BME280 与 MPU6050 共用 ESP32-S3 的 I2C 通信总线。

I2C连接方式：

```text
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

* 获取实时经纬度
* 获取定位时间
* 提供浮标空间位置数据
* 为后续轨迹记录与导航应用提供位置基础

接线：

| GPS Module | ESP32-S3  |
| ---------- | --------- |
| VCC        | 3.3V / 5V |
| GND        | GND       |
| TX         | ESP32 RX  |
| RX         | ESP32 TX  |

通信方式：

UART Serial Communication

---

## DS18B20 Waterproof Temperature Sensor

**当前状态：⏳ Waiting for Integration**

DS18B20 为后续计划接入的防水数字温度传感器，主要用于直接测量浮标周围水体温度。

计划功能：

* 水体温度检测
* 海洋环境温度数据采集
* 为后续环境分析提供水温数据
* 为海洋风险评估提供基础环境参数

当前 DS18B20 尚未正式接入 ESP32-S3，具体接线、GPIO 分配和程序模块将在后续硬件接入完成后更新。

---

# 🖥 OLED Display Module

当前版本已接入 OLED 显示模块，用于将浮标关键状态信息直接显示在样机本地屏幕上。

OLED 的加入使样机在不依赖电脑串口监视器的情况下，也能够直接查看部分实时运行信息，提高样机调试、测试和现场展示的便利性。

功能：

* 实时数据显示
* 传感器状态显示
* GPS 定位状态显示
* 浮标运行状态显示
* 本地可视化输出

接线：

| OLED Display | ESP32-S3 |
| ------------ | -------- |
| VCC          | 3.3V     |
| GND          | GND      |
| SDA          | GPIO8    |
| SCL          | GPIO9    |

OLED 使用 I2C 通信方式，与 MPU6050 和 BMP280/BME280 共用 ESP32-S3 的 I2C 总线。

当前 I2C 总线结构：

```text
ESP32-S3 GPIO8 (SDA)
        |
        |------ MPU6050 SDA
        |
        |------ BMP280/BME280 SDA
        |
        └------ OLED SDA


ESP32-S3 GPIO9 (SCL)
        |
        |------ MPU6050 SCL
        |
        |------ BMP280/BME280 SCL
        |
        └------ OLED SCL
```

各设备通过不同的 I2C 地址进行通信，使多个 I2C 设备能够共享同一组 SDA / SCL 引脚。

### Display Upgrade Note

后续计划将当前显示屏升级为：

**2.42 英寸显示屏**

当前阶段暂时保留现有显示模块的接口设计：

```text
SDA → GPIO8
SCL → GPIO9
```

因此现阶段不调整 ESP32-S3 的显示屏引脚规划。

待 2.42 英寸屏幕实际接入后，再根据具体屏幕控制芯片和驱动库，对显示分辨率、初始化参数以及界面布局进行适配。

**Upgrade Status：⏳ Planned**

---

# ☀️ Solar Power System

当前样机已经完成太阳能能源部分的基础搭建。

能源系统主要由：

* Solar Panel
* Solar Power Management Module
* Lithium Battery
* USB Power Output
* ESP32-S3

组成。

当前基础供电链路：

```text
Solar Panel
     |
     ↓
Solar Power Management Module
     |
     |------ Lithium Battery
     |
     └------ USB Power Output
                |
                ↓
             ESP32-S3
```

其中：

**太阳能板**

负责提供外部太阳能能源输入。

**太阳能电源管理模块**

负责连接太阳能板、锂电池以及系统电源输出，是当前样机能源系统的核心模块。

**锂电池**

用于能源储存，并在太阳能输入不足或没有太阳光时继续为系统提供电力。

**USB Power Output**

用于将能源模块输出连接至 ESP32-S3 开发板，为主控板及其连接的传感器提供电源。

当前状态：

* ✅ Solar Panel Connected
* ✅ Solar Power Module Connected
* ✅ Lithium Battery Connected
* ✅ USB Power Output Connected
* ✅ ESP32-S3 Power Supply Connected
* ✅ Basic Power Chain Completed

当前太阳能板、太阳能模块、锂电池与开发板之间的基础能源链路已经打通。

后续将继续进行实际光照环境下的充电能力、连续供电能力以及整机续航测试。

---

# 🔌 Complete Wiring Diagram

当前整体系统连接：

```text
                              ESP32-S3
                                  |
            ---------------------------------------------
            |                    |                      |
           I2C                  UART                   USB
            |                    |                      |
      -------------              |                      |
      |     |     |              |                      |
 MPU6050  BMP280  OLED       GPS Module         Solar Power
           /                                    System / PC
        BME280
```

I2C BUS：

```text
GPIO8 (SDA)
   |
   +------ MPU6050 SDA
   |
   +------ BMP280/BME280 SDA
   |
   +------ OLED SDA


GPIO9 (SCL)
   |
   +------ MPU6050 SCL
   |
   +------ BMP280/BME280 SCL
   |
   +------ OLED SCL
```

UART：

```text
ESP32-S3                 GPS Module

RX       <-------------      TX

TX       ------------->      RX
```

POWER：

```text
3.3V ---------------- Sensor / OLED VCC

GND  ---------------- Common GND
```

Solar Power：

```text
Solar Panel
     |
     ↓
Solar Power Management Module
     |
     |------ Lithium Battery
     |
     └------ USB Output
                |
                ↓
             ESP32-S3
```

完整硬件功能结构：

```text
                     +----------------+
                     |    ESP32-S3    |
                     +----------------+
                       |      |      |
                       |      |      |
                     I2C    UART    USB
                       |      |      |
             ----------       |      |
             |                |      |
      ---------------         |      |
      |      |      |         |      |
  MPU6050  BMP280   OLED     GPS   Power System
            /
          BME280
```

待更新模块：

```text
DS18B20 Waterproof Temperature Sensor
                |
                └── Waiting for Integration
```

---

# 🔄 System Data Flow

当前样机的数据流：

```text
MPU6050 ---------|
                 |
BMP280/BME280 ---|
                 |
                 |----> ESP32-S3 ----> Data Processing
                 |                          |
GPS -------------|                          |
                                            |
                         -------------------
                         |                 |
                         ↓                 ↓
                   Serial Monitor     OLED Display
```

ESP32-S3 作为系统核心，统一读取姿态传感器、环境传感器和 GPS 数据，并将处理后的信息输出到串口监视器和 OLED 屏幕。

未来 DS18B20 接入后，将增加独立水温数据输入：

```text
DS18B20
   |
   └------> ESP32-S3
              |
              └------> Water Temperature Data
```

---

# 💻 Software Development Environment

开发环境：

* Visual Studio Code
* PlatformIO
* Arduino Framework

当前项目结构：

```text
Buoy_Project_V1_2/
│
├── src/
│   ├── main.cpp
│   ├── oled_display.cpp
│   └── oled_display.h
│
├── include/
│
├── lib/
│
├── platformio.ini
│
└── README.md
```

主要文件说明：

### `main.cpp`

系统主程序，负责：

* 系统初始化
* 传感器读取
* GPS 数据处理
* 各功能模块协调
* OLED 显示模块调用

### `oled_display.cpp`

负责 OLED 显示功能的具体实现。

### `oled_display.h`

负责 OLED 显示模块接口定义。

### `platformio.ini`

负责 ESP32-S3 PlatformIO 项目配置及相关库依赖管理。

OLED 功能采用独立模块设计，避免将全部功能集中在 `main.cpp` 中，为后续继续增加传感器、通信和数据处理模块提供更清晰的软件结构。

---

# 📡 Current Functions

## Multi Sensor Data Acquisition

系统目前可以采集：

### Motion Data

* Acceleration X/Y/Z
* Gyroscope X/Y/Z

### Environmental Data

* Temperature
* Atmospheric Pressure
* Altitude Estimation

### Position Data

* Latitude
* Longitude
* GPS Time
* GPS Position Status

### Local Display

OLED 当前用于显示浮标关键运行信息，包括：

* Sensor Data
* GPS Status
* System Status
* Real-time Monitoring Information

### Power System

当前能源系统已经实现：

* Solar Energy Input
* Lithium Battery Connection
* Power Management
* ESP32-S3 USB Power Supply

### Pending Environmental Data

DS18B20 接入后计划增加：

* Water Temperature

---

# 📟 Serial Output Example

当前运行状态可通过串口输出：

```text
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

与此同时，关键数据可以通过 OLED 屏幕进行本地显示。

---

# 📦 Hardware List

| Component                 | Function                     | Status                    |
| ------------------------- | ---------------------------- | ------------------------- |
| ESP32-S3                  | Main Controller              | ✅ Completed               |
| MPU6050                   | Motion Detection             | ✅ Completed               |
| BMP280/BME280             | Environmental Monitoring     | ✅ Completed               |
| GPS Module                | Positioning                  | ✅ Completed               |
| OLED Display              | Local Data Visualization     | ✅ Completed               |
| 2.42-inch Display         | Future Display Upgrade       | ⏳ Planned                 |
| Solar Panel               | Solar Energy Input           | ✅ Completed               |
| Solar Power Module        | Power Management             | ✅ Completed               |
| Lithium Battery           | Energy Storage               | ✅ Connected               |
| DS18B20 Waterproof Sensor | Water Temperature Monitoring | ⏳ Waiting for Integration |
| Breadboard                | Prototype Connection         | ✅ In Use                  |
| Dupont Wires              | Hardware Connection          | ✅ In Use                  |

---

# 🔄 Version History

## Version 1.0 - Basic Sensor Platform

基础样机平台搭建。

Completed:

* ESP32-S3 controller
* MPU6050 attitude sensing
* BMP280/BME280 environmental sensing
* Basic sensor communication framework
* Serial data output

---

## Version 1.1 - GPS Positioning Integration

在基础传感器平台上加入 GPS 定位功能。

Completed:

* GPS module integration
* UART communication
* Latitude acquisition
* Longitude acquisition
* GPS positioning status
* Position data integration

---

## Version 1.2 - OLED Display Integration

在现有传感器与 GPS 系统基础上增加 OLED 本地数据显示功能。

Completed:

* OLED display integration
* Real-time local data visualization
* OLED I2C communication
* Sensor status visualization
* GPS status visualization
* Added `oled_display.cpp`
* Added `oled_display.h`
* Updated `main.cpp`
* Updated `platformio.ini`

Git Commit:

```text
Add OLED display module and update buoy interface
```

---

## Version 1.3 - Solar Power System Integration

在现有传感器、GPS 与 OLED 系统基础上增加太阳能能源系统。

Completed:

* Solar panel integration
* Solar power management module integration
* Lithium battery connection
* USB power output connection
* ESP32-S3 power supply integration
* Basic solar energy supply chain completed

当前能源结构：

```text
Solar Panel
     |
     ↓
Solar Power Management Module
     |
     |------ Lithium Battery
     |
     └------ USB Output
                |
                ↓
             ESP32-S3
```

通过太阳能板、电源管理模块和锂电池的组合，使样机开始具备脱离电脑 USB 供电后独立运行的能源基础。

Current Status:

**✅ Basic Power System Completed**

---

## Next Hardware Update

下一阶段主要进行两个硬件更新：

### 1. Display Upgrade

计划将当前显示屏升级为：

**2.42-inch Display**

当前 GPIO8 / GPIO9 I2C 接口方案暂时保留。

Status:

**⏳ Planned**

### 2. DS18B20 Waterproof Temperature Sensor

计划增加 DS18B20 防水温度传感器，用于水体温度采集。

Status:

**⏳ Waiting for Integration**

---

# 🚀 Development Roadmap

## Current Prototype

目前已经完成：

```text
ESP32-S3
   |
   |---- MPU6050
   |       └── Motion / Attitude
   |
   |---- BMP280/BME280
   |       └── Environmental Data
   |
   |---- GPS
   |       └── Positioning
   |
   |---- OLED
   |       └── Local Visualization
   |
   |---- Solar Power System
   |       ├── Solar Panel
   |       ├── Power Management
   |       └── Lithium Battery
   |
   └---- USB Serial
           └── Debug / Data Output
```

待更新：

```text
2.42-inch Display
       └── Display Upgrade

DS18B20
       └── Waterproof Water Temperature Sensor
```

当前样机已经形成：

**Sensor Acquisition → Positioning → Processing → Local Visualization**

以及：

**Solar Energy → Energy Storage → Power Management → System Power Supply**

两条基础功能链路。

---

## Version 2.0 - Data Storage and Communication

Future improvements:

* SD card data storage
* LoRa wireless communication
* Remote monitoring
* Additional marine environmental sensors
* Data logging
* Communication interface optimization

---

## Version 3.0 - Intelligent Marine Decision System

Future goals:

* Multi-source environmental data fusion
* Marine risk assessment
* Environmental risk prediction
* Dynamic route analysis
* Navigation assistance
* Intelligent buoy decision support

---

# 🎯 Project Goal

本项目目标是构建一个低成本、模块化、可扩展的智能海洋感知浮标平台。

通过 ESP32-S3 与多传感器融合技术，实现对浮标运动状态、周围环境以及地理位置的实时监测，并通过 OLED 屏幕实现关键数据的本地可视化。

同时，通过太阳能板、太阳能电源管理模块与锂电池构成独立能源系统，为浮标脱离电脑供电后持续运行提供基础能源支持。

当前样机重点完成硬件感知端、基础数据显示能力以及独立能源供电能力建设。

后续还将接入 **DS18B20 防水温度传感器**，用于增加水体温度监测能力，同时计划将现有显示模块升级为 **2.42 英寸显示屏**。

随着后续版本迭代，系统将进一步加入数据存储、无线通信以及更多海洋环境传感器，使浮标从单一的数据采集终端逐步发展为具有 **感知、定位、显示、供电、通信和数据服务能力** 的海洋智能终端。

最终，浮标采集的数据可以进一步为海洋风险分析、动态航线规划、无人航行设备以及智能导航决策系统提供基础数据支持。

---

# 👨‍💻 Development Information

Hardware Platform:

* ESP32-S3
* MPU6050
* BMP280/BME280
* GPS Module
* OLED Display
* Solar Panel
* Solar Power Management Module
* Lithium Battery
* DS18B20 Waterproof Temperature Sensor（Waiting for Integration）
* 2.42-inch Display（Planned Upgrade）

Software Platform:

* Visual Studio Code
* PlatformIO
* Arduino Framework

Current Prototype Version:

**Prototype V1.3 - Solar Power System Integration**

Current Hardware Update:

**2.42-inch Display Upgrade - Planned**

**DS18B20 Waterproof Temperature Sensor - Waiting for Integration**

Author:

Long JJ
