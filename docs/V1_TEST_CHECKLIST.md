# V1 上电测试清单

- [ ] USB 上电前再次确认所有传感器使用 3.3V，而不是误接 5V。
- [ ] MPU6050 只使用 VCC/GND/SCL/SDA；XDA/XCL/AD0/INT 当前不接。
- [ ] 280 的 CSB 接 3.3V，SDO 接 GND。
- [ ] 两个模块和 ESP32 共用 GND。
- [ ] 两个模块的 SDA 都连 GPIO8。
- [ ] 两个模块的 SCL 都连 GPIO9。
- [ ] VSCode 打开的是整个项目文件夹，不是单独 main.cpp。
- [ ] PlatformIO Build 成功。
- [ ] Upload 到 COM8 成功。
- [ ] Serial Monitor 为 115200。
- [ ] I2C 扫描看到 MPU6050 的 0x68 或 0x69。
- [ ] I2C 扫描看到 280 的 0x76 或 0x77。
- [ ] MPU6050 状态显示 OK。
- [ ] 280 被识别为 BME280 或 BMP280，并显示 OK。
- [ ] 转动模块时 roll/pitch 有明显变化。
- [ ] 280 温度和气压持续有数值。
- [ ] 连续运行 2~5 分钟没有频繁报错或重启。
