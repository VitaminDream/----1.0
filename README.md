# 🌊 Intelligent Marine Buoy Prototype

## 基于 ESP32-S3 的智能海洋环境感知浮标样机

## 📌 Project Overview

本项目是一款基于 **ESP32-S3** 开发的智能海洋环境感知浮标样机。

系统通过多种传感器对浮标周围环境及自身运行状态进行实时监测，可采集环境温度、湿度、气压、水体温度、姿态、加速度、角速度以及 GPS 位置信息。

浮标采用多传感器协同感知架构，将采集的数据统一交由 ESP32-S3 处理，并通过 **OLED 本地显示、MicroSD 数据存储以及 BLE 蓝牙无线传输**形成完整的数据采集与输出链路。

系统同时加入了软件时间戳、自动工作模式以及自适应采样机制，可根据不同运行状态调整数据采集周期。

样机还搭建了由太阳能板、电源管理模块和锂电池组成的独立供电系统，为海洋环境监测、极地环境感知、风险评估、航行辅助以及智能航线规划提供基础硬件平台和真实数据来源。

当前版本：

**Prototype V1.5**

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
|  | TX | GPIO18 |
|  | RX | GPIO17 |
| **SD Card** | CS | GPIO10 |
|  | MOSI / DI | GPIO11 |
|  | SCK / CLK | GPIO12 |
|  | MISO / DO | GPIO13 |
|  | GND | GND |

---

## 总线结构

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

I2C 总线由 MPU6050、BME280 和 OLED 共用：

```text
SDA → GPIO8
SCL → GPIO9
```

GPS 使用 UART 通信。

SD 卡使用 SPI 通信。

DS18B20 使用单总线数字通信。

---

# ☀️ Power System

样机搭建了独立供电系统：

```text
Solar Panel
     |
     ↓
Solar Power Management Module
     |
     ├──── Lithium Battery
     |
     └──── USB Output
               |
               ↓
             ESP32-S3
               |
               ↓
        Sensors / OLED / GPS
```

电源系统主要由：

- 太阳能板
- 太阳能电源管理模块
- 锂电池
- ESP32-S3
- 传感器及外围模块

组成。

太阳能板负责提供外部能源，电源管理模块负责电源管理与系统供电，锂电池用于能量储存，从而构成浮标的基础独立能源系统。

---

# ✅ Implemented Functions

当前浮标样机已经集成环境感知、姿态监测、定位、显示、存储、无线通信以及运行状态管理等功能。

---

# 🌡 Environmental Monitoring

系统使用 **BME280** 采集周围环境参数。

包括：

- 环境温度
- 空气相对湿度
- 大气压力

主要数据字段：

```text
T = Temperature
P = Pressure
H = Humidity
```

单位：

```text
Temperature → °C
Pressure    → hPa
Humidity    → %RH
```

---

# 🌊 Water Temperature Monitoring

系统使用 **DS18B20 防水数字温度传感器**采集水体温度。

主要字段：

```text
WT = Water Temperature
```

单位：

```text
°C
```

DS18B20 通过独立数字数据线连接 ESP32-S3：

```text
DATA → GPIO4
```

水温数据与其他环境数据一起进入统一的数据采集结构。

---

# 🧭 Attitude & Motion Monitoring

系统使用 **MPU6050 六轴惯性传感器**监测浮标姿态与运动状态。

可获取：

- Roll 横滚角
- Pitch 俯仰角
- X 轴加速度
- Y 轴加速度
- Z 轴加速度
- X 轴角速度
- Y 轴角速度
- Z 轴角速度

其中：

```text
Roll  → 左右方向横滚
Pitch → 前后方向俯仰
```

姿态和运动数据可以反映浮标受到波浪、晃动以及外部环境变化时的运动状态。

这些数据也可以作为自适应采样和海况状态判断的重要输入。

---

# 📍 GPS Positioning

系统接入 **WHEELTEC G60 GPS** 定位模块。

GPS 通过 UART 与 ESP32-S3 通信：

```text
GPS TX → GPIO18
GPS RX → GPIO17
```

系统可以获取：

```text
Latitude
Longitude
```

即：

```text
Lat
Lon
```

GPS 位置信息与环境传感器数据共同组成浮标的数据记录结构，为后续进行位置相关环境分析、风险评估和航线规划提供空间信息。

---

# 🕒 Time System

系统加入独立的软件时间戳机制。

程序可以设置固定起始时间，例如：

```text
2026-08-31 20:30:00
```

ESP32-S3 根据系统运行时间持续计算后续时间。

数据时间格式统一为：

```text
YYYY-MM-DD HH:MM:SS
```

例如：

```text
2026-08-31 20:51:00
2026-08-31 20:52:00
2026-08-31 20:53:00
```

时间系统用于为每一组传感器数据提供统一时间信息。

整体关系：

```text
Fixed Start Time
       |
       ↓
ESP32-S3 Runtime
       |
       ↓
Current Timestamp
       |
       ↓
Sensor Data
```

时间信息随后同步用于：

```text
OLED
MicroSD
BLE
```

保证不同数据输出端使用统一的数据时间。

---

# 💾 MicroSD Data Logging

系统通过 MicroSD 卡进行本地数据存储。

数据文件：

```text
/buoy.csv
```

采用标准 CSV 数据结构，便于后续直接导入：

- Microsoft Excel
- Python
- Pandas
- MATLAB
- 数据可视化程序
- 风险评估系统
- 航线规划算法

进行进一步处理。

---

# 📊 Data Structure

当前数据结构包括：

| Column | Field | Meaning | Unit / Value |
|---:|---|---|---|
| 1 | `time` | 数据时间 | YYYY-MM-DD HH:MM:SS |
| 2 | `lat` | 纬度 | ° |
| 3 | `lon` | 经度 | ° |
| 4 | `temp` | 环境温度 | °C |
| 5 | `pressure` | 大气压力 | hPa |
| 6 | `humidity` | 相对湿度 | %RH |
| 7 | `water_temp` | 水体温度 | °C |
| 8 | `roll` | 横滚角 | ° |
| 9 | `pitch` | 俯仰角 | ° |
| 10 | `mode` | 系统工作模式 | AUTO |
| 11 | `state` | 当前运行状态 | NORMAL / CALM / ALERT |
| 12 | `interval` | 当前采样时间间隔 | s |
| 13 | `status` | 系统状态信息 | Status |

数据结构不仅保存传感器观测值，同时保存浮标当前运行模式和采样状态。

这样在后续分析数据时，可以同时知道：

```text
测到了什么
+
什么时候测的
+
浮标处于什么状态
+
当时使用什么采样频率
```

---

# 📡 BLE Wireless Transmission

系统加入 **Bluetooth Low Energy（BLE）** 无线数据传输功能。

ESP32-S3 可以通过 BLE 将最新的浮标数据发送至手机或其他 BLE 接收设备。

BLE 数据结构例如：

```text
2026-08-31 20:55:00,
T=25.94,
P=1002.98,
H=45.1,
WT=25.37,
R=-0.76,
Pi=-0.15,
Lat=NA,
Lon=NA,
M=AUTO,
S=NORMAL,
I=60,
SD=OK
```

字段含义：

| Field | Meaning |
|---|---|
| `T` | Temperature |
| `P` | Atmospheric Pressure |
| `H` | Relative Humidity |
| `WT` | Water Temperature |
| `R` | Roll |
| `Pi` | Pitch |
| `Lat` | Latitude |
| `Lon` | Longitude |
| `M` | Working Mode |
| `S` | Sampling / System State |
| `I` | Sampling Interval |
| `SD` | SD Card Status |

例如：

```text
M=AUTO
S=NORMAL
I=60
SD=OK
```

表示：

```text
Automatic Mode
+
Normal State
+
60 s Sampling Interval
+
SD Card Normal
```

BLE 的加入使浮标除了本地 SD 数据保存之外，还具备近距离无线数据读取能力。

---

# 🖥 OLED Display

系统使用 **2.42-inch OLED** 作为浮标本地数据显示终端。

OLED 可显示：

- 系统时间
- 环境温度
- 空气湿度
- 大气压力
- 水体温度
- Roll 横滚角
- Pitch 俯仰角
- 三轴加速度
- GPS 状态
- SD 卡状态
- 系统运行状态

OLED 显示结构示例：

```text
环境 温23.1C 气997
湿度57.7% 水温27.1C
姿态 横-0.2 俯+0.8
加速 X-0.2 Y-0.0 Z10.5
定位 GPS状态
存储 SD卡正常
```

数据来源：

```text
环境温度
湿度
气压
   ↓
BME280


水温
 ↓
DS18B20


姿态
加速度
角速度
   ↓
MPU6050


位置
 ↓
WHEELTEC G60 GPS


存储状态
   ↓
MicroSD
```

OLED 为浮标提供本地实时状态界面，可直接观察系统运行情况。

---

# 🔄 Unified Data Acquisition

系统采用统一数据采集架构。

整体数据流：

```text
                  Sensors
                     |
                     ↓
                 ESP32-S3
                     |
                     ↓
              Sensor Reading
                     |
                     ↓
             Unified Data Frame
                     |
        ┌────────────┼────────────┐
        │            │            │
        ↓            ↓            ↓
      OLED        MicroSD        BLE
     Display      CSV Log      Wireless
```

环境传感器、姿态传感器、水温传感器和 GPS 数据经过 ESP32-S3 统一处理。

处理后的数据可以同时用于：

- OLED 本地显示
- MicroSD 长期数据记录
- BLE 无线数据发送

形成统一的数据采集与输出结构。

---

# ⚙️ Working Mode

系统加入工作模式管理机制。

主要运行信息包括：

```text
Mode
State
Interval
```

---

## Mode

系统支持自动工作模式：

```text
AUTO
```

即：

```text
Automatic Mode
```

在自动模式下，系统可以根据当前运行状态选择对应的数据采样策略。

---

## State

系统状态按照环境及浮标运行情况划分为：

```text
CALM
NORMAL
ALERT
```

分别对应：

```text
CALM
长期平静状态

NORMAL
正常工作状态

ALERT
高变化 / 高频监测状态
```

---

## Interval

`Interval` 表示当前数据采集时间间隔。

例如：

```text
I=60
```

表示：

```text
60 seconds / sample
```

即每 1 分钟进行一次数据采集。

---

# ⏱ Adaptive Sampling

为了同时兼顾数据有效性、变化捕获能力以及能源利用效率，系统采用自适应采样设计。

采样策略划分为三种状态：

| State | Environment | Sampling Interval |
|---|---|---:|
| `CALM` | 长期平静环境 | 600 s |
| `NORMAL` | 正常工作环境 | 60 s |
| `ALERT` | 环境快速变化 | 1 s |

对应：

```text
CALM
10 min / sample
```

```text
NORMAL
1 min / sample
```

```text
ALERT
1 s / sample
```

整体逻辑：

```text
               Environment
                    |
                    ↓
              State Analysis
                    |
       ┌────────────┼────────────┐
       │            │            │
       ↓            ↓            ↓
     CALM         NORMAL       ALERT
       │            │            │
       ↓            ↓            ↓
     600 s          60 s          1 s
       │            │            │
       └────────────┼────────────┘
                    ↓
              Data Acquisition
```

这种机制可以使浮标：

**环境平静时减少重复数据采集，正常情况下保持稳定监测，环境变化明显时提高时间分辨率。**

---

# 🌙 Power-Aware Sampling

自适应采样同时与浮标能源管理相结合。

在长时间平静状态下：

```text
CALM
↓
600 s / sample
```

可以减少：

- 传感器读取次数
- SD 卡写入次数
- BLE 数据发送次数
- ESP32-S3 高频运行时间

正常状态：

```text
NORMAL
↓
60 s / sample
```

在数据量和监测连续性之间保持平衡。

环境快速变化时：

```text
ALERT
↓
1 s / sample
```

提高采样频率，以获得更高时间分辨率的数据。

形成：

```text
Environment State
       |
       ↓
Sampling Strategy
       |
       ↓
Data Resolution
       +
Energy Management
```

协同工作的运行方式。

---

# 🧩 System Architecture

当前浮标整体架构：

```text
                         ┌──────────────────┐
                         │     ESP32-S3     │
                         │ Main Controller  │
                         └────────┬─────────┘
                                  │
          ┌───────────────────────┼──────────────────────┐
          │                       │                      │
          ▼                       ▼                      ▼
       MPU6050                 BME280                 DS18B20
   Attitude / Motion        Environment             Water Temp
          │                       │                      │
          └───────────────────────┼──────────────────────┘
                                  │
                                  │
                              GPS Data
                                  │
                                  ▼
                         Unified Sensor Data
                                  │
                                  ▼
                           Time Timestamp
                                  │
                                  ▼
                          State Management
                                  │
                                  ▼
                         Adaptive Sampling
                                  │
                 ┌────────────────┼────────────────┐
                 │                │                │
                 ▼                ▼                ▼
               OLED           MicroSD            BLE
              Display          CSV Log       Wireless Data
                                  │
                                  ▼
                            Data Processing
                                  │
                                  ▼
                         Environmental Analysis
                                  │
                                  ▼
                           Risk Assessment
                                  │
                                  ▼
                         Navigation Decision
                                  │
                                  ▼
                       Dynamic Route Planning
```

---

# 📁 Project Structure

项目采用 PlatformIO 标准工程结构：

```text
Buoy_Project_V1_2/
│
├── include/
│   ├── config.h
│   ├── oled_display.h
│   └── ...
│
├── src/
│   ├── main.cpp
│   ├── oled_display.cpp
│   └── ...
│
├── lib/
│
├── test/
│
├── platformio.ini
│
└── README.md
```

---

## `src/main.cpp`

系统主程序。

主要负责：

- ESP32-S3 系统初始化
- 多传感器数据采集
- GPS 数据处理
- 软件时间管理
- 统一数据组织
- 工作模式管理
- 状态管理
- 采样间隔管理
- MicroSD 数据记录
- BLE 数据发送
- OLED 数据更新
- 主程序任务调度

---

## `include/config.h`

系统统一配置文件。

主要用于管理：

- GPIO 引脚
- I2C 配置
- UART 配置
- SPI 配置
- 传感器地址
- 时间参数
- 采样参数
- 系统运行参数

---

## `include/oled_display.h`

OLED 显示模块接口定义。

---

## `src/oled_display.cpp`

OLED 显示模块实现。

负责：

- 数据页面组织
- 环境数据显示
- 姿态数据显示
- GPS 状态显示
- SD 状态显示
- 系统状态显示

---

## `platformio.ini`

PlatformIO 工程配置文件。

主要包含：

- ESP32-S3 开发板配置
- Arduino Framework
- 编译环境
- 串口配置
- 项目依赖库

---

# 🛠 Hardware List

| Hardware | Function |
|---|---|
| **ESP32-S3** | 系统主控制器 |
| **MPU6050** | 姿态、加速度和角速度监测 |
| **BME280** | 环境温度、湿度和气压监测 |
| **DS18B20** | 防水水体温度监测 |
| **WHEELTEC G60 GPS** | 经纬度定位 |
| **2.42-inch OLED** | 本地数据显示 |
| **MicroSD Module** | CSV 数据存储 |
| **BLE** | 无线数据传输 |
| **Solar Panel** | 太阳能输入 |
| **Solar Power Management Module** | 电源管理 |
| **Lithium Battery** | 能量储存 |

---

# 💻 Development Environment

开发环境：

```text
Visual Studio Code
+
PlatformIO
+
Arduino Framework
```

主控制器：

```text
ESP32-S3
```

版本管理：

```text
Git
+
GitHub
```

---

# 🎯 Project Application

本项目面向海洋环境感知与智能航行应用。

浮标作为整个系统的前端感知节点，主要负责：

```text
Real Environment
       ↓
Environmental Sensing
       ↓
Motion Sensing
       ↓
Position Information
       ↓
Timestamp
       ↓
Data Storage
       ↓
Wireless Transmission
```

采集得到的数据可以进一步进入软件系统：

```text
Buoy Sensor Data
       ↓
Data Processing
       ↓
Environmental Analysis
       ↓
Risk Assessment
       ↓
Navigation Decision
       ↓
Dynamic Route Planning
```

---

# 🌐 Application Scenarios

系统可面向：

- 海洋环境监测
- 极地环境感知
- 海冰区域环境数据采集
- 水体温度监测
- 浮标运动状态监测
- 海况变化分析
- 海洋风险评估
- 船舶航行辅助
- 动态航线规划
- 智能导航
- 多浮标协同环境感知

---

# 🔬 Prototype System

当前样机构成：

```text
Multi-Sensor Acquisition
          +
Environmental Monitoring
          +
Attitude Monitoring
          +
Water Temperature
          +
GPS Positioning
          +
Software Timestamp
          +
OLED Display
          +
MicroSD Logging
          +
BLE Transmission
          +
Automatic Mode
          +
Adaptive Sampling
          +
Solar Power System
```

整体形成：

```text
感知
 ↓
处理
 ↓
状态判断
 ↓
自适应采样
 ↓
显示
 ↓
存储
 ↓
无线传输
 ↓
数据应用
```

完整的智能海洋环境感知浮标样机架构。

---

# 📌 Version

## Prototype V1.5

### V1.5 Update

在 Prototype V1.4 基础上进一步加入：

- ✅ BLE 蓝牙无线数据传输
- ✅ BLE 传感器数据通知
- ✅ 软件时间戳系统
- ✅ 统一数据时间格式
- ✅ AUTO 自动工作模式
- ✅ State 系统状态记录
- ✅ Interval 采样间隔记录
- ✅ NORMAL 60 秒采样策略
- ✅ CALM / NORMAL / ALERT 自适应采样架构
- ✅ OLED / SD / BLE 统一数据流程
- ✅ 数据采集与能源管理协同设计

核心数据链路：

```text
Sensors
   ↓
ESP32-S3
   ↓
Unified Data
   ↓
State Management
   ↓
Adaptive Sampling
   ↓
┌─────────┬─────────┬─────────┐
OLED      SD        BLE
```

---

**Prototype V1.5**

**Updated: 2026-08-31**
