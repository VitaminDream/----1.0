# 🌊 Intelligent Marine Buoy Prototype

## 基于 ESP32-S3 的智能海洋环境感知浮标样机

## 📌 Project Overview

本项目是一款基于 **ESP32-S3** 开发的智能海洋环境感知浮标样机。

系统通过多种传感器对浮标周围环境及自身运行状态进行实时监测，目前能够采集环境温度、湿度、气压、水体温度、姿态、加速度以及 GPS 位置信息，并通过 OLED 屏幕进行本地实时显示，同时将主要数据自动保存到 SD 卡。

样机同时搭建了由太阳能板、电源管理模块和锂电池组成的基础独立供电系统，为后续海洋环境监测、风险评估、航行辅助及智能导航应用提供基础硬件平台。

当前版本：

**Prototype V1.4**

---

# 🔌 Hardware Wiring

## ESP32-S3 引脚连接

| 模块 | 模块引脚 | ESP32-S3 |
|---|---|---|
| **MPU6050** | VCC | 3.3V |
|  | GND | GND |
|  | SDA | GPIO8 |
|  | SCL | GPIO9 |
| **BME280** | VCC | 3.3V |
|  | GND | GND |
|  | SDA | GPIO8 |
|  | SCL | GPIO9 |
|  | CSB | 3.3V |
|  | SDO | GND |
| **DS18B20** | VCC / + | 3.3V |
|  | GND / - | GND |
|  | DATA / D | GPIO4 |
| **2.42-inch OLED** | VCC | 3.3V |
|  | GND | GND |
|  | SDA | GPIO8 |
|  | SCL | GPIO9 |
| **WHEELTEC G60 GPS** | GND | GND |
|  | VIN | 5V |
|  | PPS | 暂不接 |
|  | RESET | 暂不接 |
|  | TX | GPIO18 |
|  | RX | GPIO17 |
| **SD Card** | CS | GPIO10 |
|  | MOSI / DI | GPIO11 |
|  | SCK / CLK | GPIO12 |
|  | MISO / DO | GPIO13 |
|  | GND | GND |

### 总线结构

```text
ESP32-S3

GPIO4
└── DS18B20 DATA


GPIO8  SDA
├── MPU6050
├── BME280
└── OLED

GPIO9  SCL
├── MPU6050
├── BME280
└── OLED


GPIO10 ── SD CS
GPIO11 ── SD MOSI / DI
GPIO12 ── SD SCK / CLK
GPIO13 ── SD MISO / DO


GPIO17 ── GPS RX
GPIO18 ── GPS TX
```

### 电源系统

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

---

# ✅ Implemented Functions

当前样机已经实现以下功能：

### 🌡 环境监测

- BME280 环境温度检测
- BME280 湿度检测
- BME280 大气压力检测
- DS18B20 防水水温检测

### 🧭 姿态与运动监测

- MPU6050 横滚角检测
- MPU6050 俯仰角检测
- X / Y / Z 三轴加速度检测
- X / Y / Z 三轴角速度检测

### 📍 GPS 定位

- GPS 经纬度获取
- GPS 定位状态监测
- 卫星搜索状态显示

### 💾 数据存储

SD 卡可以自动保存实时采集数据：

```text
/buoy.csv
```

当前 CSV 数据包括：

```text
time
lat
lon
temp
pressure
humidity
water_temp
roll
pitch
```

### 🖥 本地实时显示

2.42 英寸 OLED 可以实时显示：

- 环境温度
- 湿度
- 气压
- 水温
- 横滚角
- 俯仰角
- 三轴加速度
- GPS 状态
- SD 卡状态

### ☀️ 独立供电

当前已经完成：

- 太阳能板接入
- 太阳能电源管理
- 锂电池储能
- ESP32-S3 基础独立供电链路

---

# 🖥 OLED Display Example

OLED 屏幕实时显示效果大致如下：

```text
环境 温23.1C 气997
湿度57.7% 水温27.1C
姿态 横-0.2 俯+0.8
加速 X-0.2 Y-0.0 Z10.5
定位 搜星中
存储 SD卡正常
```

其中：

```text
环境温度 / 湿度 / 气压 → BME280

水温                   → DS18B20

姿态 / 加速度           → MPU6050

定位                   → WHEELTEC G60 GPS

存储                   → SD Card
```

当 GPS 获得有效卫星定位后，系统将由：

```text
定位 搜星中
```

变为已定位状态，并开始获得实际经纬度信息。

---

## Current Version

**Prototype V1.4**
