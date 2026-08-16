#include <Arduino.h>
#include <Wire.h>
#include <TinyGPS++.h>

#include "config.h"
#include "i2c_utils.h"
#include "sensor_data.h"
#include "sensors.h"

// ============================================================
// 浮标样机 V1.1
// ESP32-S3 + MPU6050 + BMP280/BME280 + WHEELTEC G60
// ============================================================
//
// MPU6050 + 280：
// SDA -> GPIO8
// SCL -> GPIO9
//
// G60：
// G60 TX  -> ESP32 GPIO18
// G60 RX  -> ESP32 GPIO17（第一版可以暂时不接）
// G60 VIN -> 5V
// G60 GND -> GND
// PPS / RESET 暂时不接
//
// ============================================================


// ============================================================
// G60 串口配置
// ============================================================

// ESP32 RX，接收 G60 TX
constexpr int GPS_RX_PIN = 18;

// ESP32 TX，连接 G60 RX
// 如果你现在没有接 G60 RX，也没关系
constexpr int GPS_TX_PIN = 17;

// G60 默认波特率
constexpr uint32_t GPS_BAUD = 9600;


// ============================================================
// 硬件对象
// ============================================================

BuoySensors sensors;

// 使用 ESP32-S3 的 UART1
HardwareSerial G60Serial(1);

// GPS 数据解析器
TinyGPSPlus gps;


// ============================================================
// 时间控制
// ============================================================

uint32_t lastSampleMs = 0;

bool gpsWarningShown = false;


// ============================================================
// 持续读取 G60
// ============================================================

void serviceGps() {

  while (G60Serial.available() > 0) {

    char c = G60Serial.read();

    gps.encode(c);
  }
}


// ============================================================
// MPU6050 输出
// ============================================================

void printImu(const ImuData &imu) {

  if (!imu.valid) {

    Serial.println("IMU  | MPU6050: N/A");

    return;
  }

  Serial.printf(
      "IMU  | Roll=%+.2f deg  Pitch=%+.2f deg\n",
      imu.rollDeg,
      imu.pitchDeg
  );

  Serial.printf(
      "     | Acc X=%+.2f  Y=%+.2f  Z=%+.2f m/s2\n",
      imu.ax,
      imu.ay,
      imu.az
  );

  Serial.printf(
      "     | Gyro X=%+.2f  Y=%+.2f  Z=%+.2f rad/s\n",
      imu.gx,
      imu.gy,
      imu.gz
  );

  Serial.printf(
      "     | DynamicAcc=%.3f  Temp=%.2f C\n",
      imu.dynamicAccel,
      imu.temperatureC
  );
}


// ============================================================
// BMP280 / BME280 输出
// ============================================================

void printEnvironment(const EnvData &env) {

  if (!env.valid) {

    Serial.println("ENV  | 280 sensor: N/A");

    return;
  }

  Serial.printf(
      "ENV  | Temp=%.2f C  Pressure=%.2f hPa",
      env.temperatureC,
      env.pressureHpa
  );

  // 如果你的紫色280是 BME280，则显示湿度
  if (env.hasHumidity && isfinite(env.humidityPct)) {

    Serial.printf(
        "  Humidity=%.1f%%",
        env.humidityPct
    );
  }

  Serial.println();

  Serial.printf(
      "     | Altitude~=%.2f m\n",
      env.altitudeM
  );
}


// ============================================================
// G60 GPS 输出
// ============================================================

void printGps() {

  Serial.print("GPS  | ");

  // 有有效经纬度，而且数据不是旧数据
  bool positionValid =
      gps.location.isValid() &&
      gps.location.age() < 5000;

  if (positionValid) {

    Serial.println("FIX");

    // 经纬度
    Serial.printf(
        "     | Lat=%.6f  Lon=%.6f\n",
        gps.location.lat(),
        gps.location.lng()
    );


    // 卫星数
    Serial.print("     | Satellites=");

    if (gps.satellites.isValid()) {

      Serial.print(gps.satellites.value());

    } else {

      Serial.print("--");
    }


    // 速度
    Serial.print("  Speed=");

    if (gps.speed.isValid()) {

      Serial.print(gps.speed.kmph(), 2);

      Serial.print(" km/h");

    } else {

      Serial.print("--");
    }


    // GPS海拔
    Serial.print("  GPS Alt=");

    if (gps.altitude.isValid()) {

      Serial.print(gps.altitude.meters(), 1);

      Serial.print(" m");

    } else {

      Serial.print("--");
    }

    Serial.println();


    // HDOP
    Serial.print("     | HDOP=");

    if (gps.hdop.isValid()) {

      Serial.print(gps.hdop.hdop(), 1);

    } else {

      Serial.print("--");
    }


    // 移动方向
    Serial.print("  Course=");

    if (gps.course.isValid()) {

      Serial.print(gps.course.deg(), 1);

      Serial.print(" deg");

    } else {

      Serial.print("--");
    }

    Serial.println();
  }

  // 已经收到 G60 数据，但是还没定位
  else if (gps.charsProcessed() > 10) {

    Serial.print("SEARCHING");

    Serial.print("  | Characters=");

    Serial.print(gps.charsProcessed());


    if (gps.satellites.isValid()) {

      Serial.print("  Satellites=");

      Serial.print(gps.satellites.value());
    }

    Serial.println();

    Serial.println(
        "     | G60 data received, waiting for satellites..."
    );
  }

  // 连数据都没收到
  else {

    Serial.println("NO DATA");

    Serial.println(
        "     | Check G60 VIN/GND/TX -> GPIO18"
    );
  }
}


// ============================================================
// setup
// ============================================================

void setup() {

  // ==========================================================
  // USB 串口
  // ==========================================================

  Serial.begin(BuoyConfig::SERIAL_BAUD);

  delay(1200);


  Serial.println();

  Serial.println(
      "=================================================="
  );

  Serial.println(
      " Buoy Prototype V1.1"
  );

  Serial.println(
      " MPU6050 + 280 + WHEELTEC G60 GNSS"
  );

  Serial.println(
      "=================================================="
  );


  // ==========================================================
  // I2C
  // ==========================================================

  if (!Wire.begin(
          BuoyConfig::I2C_SDA_PIN,
          BuoyConfig::I2C_SCL_PIN,
          BuoyConfig::I2C_FREQUENCY_HZ)) {

    Serial.println(
        "[FATAL] I2C start failed."
    );

    return;
  }


  Serial.println(
      "[I2C] SDA=GPIO8  SCL=GPIO9"
  );


  // 扫描 MPU6050 和 280
  I2CUtils::scanBus(Wire, Serial);


  // 启动 MPU6050 + 280
  sensors.begin(
      Wire,
      Serial
  );


  // ==========================================================
  // G60 UART
  // ==========================================================

  G60Serial.begin(
      GPS_BAUD,
      SERIAL_8N1,
      GPS_RX_PIN,
      GPS_TX_PIN
  );


  Serial.println();

  Serial.println(
      "[G60] UART started"
  );

  Serial.println(
      "[G60] Baud = 9600"
  );

  Serial.println(
      "[G60] G60 TX -> ESP32 GPIO18"
  );

  Serial.println(
      "[G60] ESP32 GPIO17 -> G60 RX (optional)"
  );


  Serial.println();

  Serial.println(
      "--------------------------------------------------"
  );

  Serial.println(
      "[READY] Buoy Prototype V1.1 running"
  );

  Serial.println(
      "--------------------------------------------------"
  );
}


// ============================================================
// loop
// ============================================================

void loop() {

  // GPS必须持续读取
  // 不要放到每秒一次的采样代码里面
  serviceGps();


  uint32_t nowMs = millis();


  // ==========================================================
  // 开机10秒仍完全收不到GPS数据
  // ==========================================================

  if (!gpsWarningShown &&
      nowMs > 10000 &&
      gps.charsProcessed() < 10) {

    Serial.println();

    Serial.println(
        "[GPS WARNING]"
    );

    Serial.println(
        "No G60 serial data received."
    );

    Serial.println(
        "Check wiring:"
    );

    Serial.println(
        "G60 VIN -> 5V"
    );

    Serial.println(
        "G60 GND -> GND"
    );

    Serial.println(
        "G60 TX  -> GPIO18"
    );

    Serial.println();

    gpsWarningShown = true;
  }


  // ==========================================================
  // 每秒读取并显示一次全部传感器
  // ==========================================================

  if (nowMs - lastSampleMs >=
      BuoyConfig::SAMPLE_INTERVAL_MS) {

    lastSampleMs = nowMs;


    ImuData imu;

    EnvData env;


    sensors.readImu(imu);

    sensors.readEnvironment(env);


    Serial.println();

    Serial.println(
        "=============== BUOY DATA ==============="
    );


    printImu(imu);

    printEnvironment(env);

    printGps();


    Serial.println(
        "========================================="
    );
  }


  // 给系统一点时间处理其他任务
  delay(2);
}