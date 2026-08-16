# 浮标样机 V1：ESP32-S3 + MPU6050 + BME280/BMP280

这是你的第一版浮标传感器程序，目标不是一次把所有功能都塞进去，而是先把 **两个传感器稳定跑起来**，同时把程序结构留好，后面可以继续加 GPS、通信、SD 卡、网页/3D 驾驶舱等模块。

## V1 完成什么

1. ESP32-S3 使用 I2C：`SDA=GPIO8`、`SCL=GPIO9`。
2. 开机自动扫描 I2C 总线。
3. 自动寻找 MPU6050：`0x68` / `0x69`。
4. 自动识别紫色 280 模块到底是 **BME280** 还是 **BMP280**，并支持 `0x76` / `0x77`。
5. 每秒读取一次传感器。
6. MPU6050 输出：
   - X/Y/Z 加速度（m/s²）
   - X/Y/Z 角速度（rad/s）
   - 芯片温度
   - 第一版 roll / pitch 倾角估算
   - 总加速度和动态加速度指标
7. 280 输出：
   - 温度
   - 气压
   - 粗略高度估计
   - 如果识别为 BME280，再输出湿度
8. 同时输出：
   - 人能直接看的 `[DATA]` 行
   - 后面电脑端程序可以解析的 `JSON:` 行

> V1 的 roll/pitch 是根据加速度方向计算的静态/低速倾角，不是最终的姿态融合算法。浮标剧烈运动时它会受线性加速度影响。后续版本再升级互补滤波 / AHRS。

---

## 当前接线

### MPU6050

| MPU6050 | ESP32-S3 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |
| XDA | 不接 |
| XCL | 不接 |
| AD0 | 暂时不接 |
| INT | 暂时不接 |

### BME280 / BMP280 六针模块

| 280 模块 | ESP32-S3 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |
| CSB | 3.3V |
| SDO | GND |

如果你的 SDO 接 GND，280 常见 I2C 地址是 `0x76`；程序仍然会同时检查 `0x76` 和 `0x77`。

---

## VSCode 第一次使用

### 1. 安装扩展

打开 VSCode → Extensions → 搜索并安装：

`PlatformIO IDE`

安装完成后重启 VSCode。

### 2. 打开项目

不要只打开 `main.cpp`。

在 VSCode 选择：

`File -> Open Folder`

然后选择整个：

`BuoyPrototype_V1_DualSensor`

看到根目录里的 `platformio.ini` 就说明打开对了。

### 3. 接上开发板

当前配置默认：

- 上传串口：`COM8`
- 串口监视器：`COM8`
- 波特率：`115200`

如果以后 Windows 给开发板分配了别的 COM 号，只改 `platformio.ini` 里的：

```ini
upload_port = COM8
monitor_port = COM8
```

### 4. 编译

VSCode 底部 PlatformIO 工具栏点击 **✓ Build**。

第一次编译会自动下载 ESP32 Arduino 框架和 Adafruit 库，所以时间会比后面长。

### 5. 烧录

点击 **→ Upload**。

如果上传时一直连接失败，可以在出现 `Connecting...` 时按一下开发板 BOOT 键；若正常上传则不需要按。

### 6. 看串口数据

点击 PlatformIO 的 **Serial Monitor**。

波特率已经设置为 `115200`。

正常开机应该先看到类似：

```text
[I2C] Found device at 0x68
[I2C] Found device at 0x76
[OK] MPU6050 ready at 0x68
[OK] BME280 ready at 0x76
```

如果紫色模块实际上是 BMP280，则会显示：

```text
[OK] BMP280 ready at 0x76
```

随后每秒会看到：

```text
[DATA] 5023 ms | roll=+1.20 deg pitch=-0.80 deg | acc=9.801 m/s^2 dyn=0.006 | envT=26.42 C press=1008.31 hPa hum=63.2% alt~=41.20 m
JSON:{"version":1,...}
```

---

## 第一次测试你只需要看 3 件事

### A. I2C 扫描

理想情况看到两个地址，例如：

- `0x68`：MPU6050
- `0x76`：BME280/BMP280

### B. 轻轻转动 MPU6050

`roll`、`pitch` 应该明显变化。

### C. 用手捂一下 280 模块

温度会缓慢变化；如果是 BME280，湿度也通常会变化。

---

## 出问题时不要乱改代码

### 只找到 0x68

说明 MPU6050 通了，280 没通。重点查 280 的 VCC/GND/SDA/SCL/CSB/SDO。

### 只找到 0x76 或 0x77

说明 280 通了，MPU6050 没通。重点检查 MPU6050 是否误接到了 `XCL/XDA`，真正需要的是 `SCL/SDA`。

### 一个地址都没有

优先检查：

1. ESP32 的 GPIO8 / GPIO9 是否接反；
2. 两块传感器是否与 ESP32 共地；
3. 是否确实使用 3.3V；
4. 面包板电源轨是否被中间断开；
5. 杜邦线有没有插错排。

### 出现 0x68 和 0x76，但程序提示传感器无法初始化

把串口完整内容发给 ChatGPT，不要先大范围换线。I2C 地址能扫描出来已经说明供电和总线大体正确，下一步应该针对芯片型号或库初始化排查。

---

## 文件结构

```text
BuoyPrototype_V1_DualSensor/
├─ platformio.ini          # PlatformIO / ESP32-S3 / COM8 / 库配置
├─ README.md               # 本说明
├─ include/
│  ├─ config.h             # 引脚、采样周期、地址等总配置
│  ├─ sensor_data.h        # 统一数据结构
│  ├─ i2c_utils.h          # I2C 工具
│  ├─ sensors.h            # 传感器模块接口
│  └─ telemetry.h          # 串口输出接口
└─ src/
   ├─ main.cpp             # 主程序，只负责组织流程
   ├─ i2c_utils.cpp        # I2C 扫描/寄存器读取
   ├─ sensors.cpp          # MPU6050 + BME/BMP280 初始化与采集
   └─ telemetry.cpp        # 人类可读 + JSON 数据输出
```

这种分法是为了后面不需要推翻 V1。后续可以继续增加：

- GPS 模块
- LoRa / Wi-Fi / 4G 通信
- SD 卡记录
- 电池电压
- 水温 / 水质传感器
- 电脑端实时可视化
- 3D 浮标姿态
- 数据保存、异常报警

而 `main.cpp` 不需要变成几千行。

---

## V1 暂时不做的内容

为了先验证样机核心链路，这一版刻意不加入：

- Wi-Fi
- MQTT
- GPS
- SD 卡
- 云端
- 网页仪表盘
- 3D 模型
- 精确波高算法
- AHRS 姿态融合

第一阶段通过标准只有一个：**两个传感器稳定被识别，并持续输出合理数据。**
