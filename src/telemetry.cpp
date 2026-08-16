#include "telemetry.h"
#include <math.h>

namespace {

void printJsonFloat(Stream &out, float value, uint8_t digits = 3) {
  if (isfinite(value)) {
    out.print(value, digits);
  } else {
    out.print("null");
  }
}

}  // namespace

namespace Telemetry {

void printBootBanner(Stream &out) {
  out.println();
  out.println("======================================");
  out.println(" Buoy Prototype V1");
  out.println(" ESP32-S3 + MPU6050 + BME280");
  out.println("======================================");
}

void printStatus(Stream &out,
                 bool imuReady, uint8_t imuAddress,
                 bool envReady, const char *envType, uint8_t envAddress) {

  out.println("[STATUS]");

  if (imuReady) {
    out.printf("MPU6050 : OK  0x%02X\n", imuAddress);
  } else {
    out.println("MPU6050 : OFFLINE");
  }

  if (envReady) {
    out.printf("%s : OK  0x%02X\n", envType, envAddress);
  } else {
    out.println("ENV280 : OFFLINE");
  }

  out.println();
}

void printHumanReadable(Stream &out,
                        uint32_t nowMs,
                        const ImuData &imu,
                        const EnvData &env) {

  out.printf("[%lu ms]\n", static_cast<unsigned long>(nowMs));

  // MPU6050
  if (imu.valid) {
    out.printf("IMU | Roll:%+.1f deg  Pitch:%+.1f deg  Motion:%.2f\n",
               imu.rollDeg,
               imu.pitchDeg,
               imu.dynamicAccel);
  } else {
    out.println("IMU | OFFLINE");
  }

  // BME280
  if (env.valid) {
    out.printf("ENV | Temp:%.1f C  Hum:",
               env.temperatureC);

    if (env.hasHumidity && isfinite(env.humidityPct)) {
      out.printf("%.1f %%", env.humidityPct);
    } else {
      out.print("N/A");
    }

    out.printf("  Press:%.1f hPa\n",
               env.pressureHpa);
  } else {
    out.println("ENV | OFFLINE");
  }

  out.println();
}


// =====================================================
// 完整 JSON 函数仍然保留
// 以后做网页、CSV、上位机时还可以继续使用
// =====================================================

void printJson(Stream &out,
               uint32_t nowMs,
               const ImuData &imu,
               const EnvData &env,
               const char *envType) {

  out.print("JSON:{\"version\":1,\"ms\":");
  out.print(nowMs);

  out.print(",\"imu_ok\":");
  out.print(imu.valid ? "true" : "false");

  out.print(",\"ax\":");
  printJsonFloat(out, imu.ax);

  out.print(",\"ay\":");
  printJsonFloat(out, imu.ay);

  out.print(",\"az\":");
  printJsonFloat(out, imu.az);

  out.print(",\"gx\":");
  printJsonFloat(out, imu.gx);

  out.print(",\"gy\":");
  printJsonFloat(out, imu.gy);

  out.print(",\"gz\":");
  printJsonFloat(out, imu.gz);

  out.print(",\"roll\":");
  printJsonFloat(out, imu.rollDeg, 2);

  out.print(",\"pitch\":");
  printJsonFloat(out, imu.pitchDeg, 2);

  out.print(",\"acc_mag\":");
  printJsonFloat(out, imu.accelMagnitude);

  out.print(",\"dyn_acc\":");
  printJsonFloat(out, imu.dynamicAccel);

  out.print(",\"imu_temp_c\":");
  printJsonFloat(out, imu.temperatureC, 2);


  out.print(",\"env_ok\":");
  out.print(env.valid ? "true" : "false");

  out.print(",\"env_type\":\"");
  out.print(envType);
  out.print("\"");

  out.print(",\"env_temp_c\":");
  printJsonFloat(out, env.temperatureC, 2);

  out.print(",\"pressure_hpa\":");
  printJsonFloat(out, env.pressureHpa, 2);

  out.print(",\"humidity_pct\":");
  printJsonFloat(out, env.humidityPct, 1);

  out.print(",\"altitude_m\":");
  printJsonFloat(out, env.altitudeM, 2);

  out.println("}");
}

}  // namespace Telemetry