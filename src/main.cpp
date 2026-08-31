#include <Arduino.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <esp_timer.h>

// ESP32-S3 BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "config.h"
#include "i2c_utils.h"
#include "sensor_data.h"
#include "sensors.h"
#include "ds18b20.h"
#include "oled_display.h"

// =====================================================
// 浮标样机 V1.6
//
// 本版核心变化：
// 1. 时间不再读取 GPS。
// 2. 每次上电/RESET 后，从固定北京时间
//    2026-08-31 20:50:00 开始连续计时。
// 3. GPS 仅用于定位、卫星数等，不参与时间。
// 4. 增加 AUTO / HIGH / NORMAL / CALM 四种控制模式。
// 5. HIGH   ：完整采集 1 秒/次
// 6. NORMAL ：完整采集 1 分钟/次
// 7. CALM   ：完整采集 10 分钟/次
// 8. AUTO   ：MPU6050 每秒轻量监测，自动切换前三种状态。
// 9. BLE 特征增加 WRITE，可写入 AUTO/HIGH/NORMAL/CALM。
// 10. SD / BLE / 串口统一使用同一套固定起始时间。
// =====================================================


// =====================================================
// GPS
// =====================================================

constexpr int GPS_RX_PIN = 18;   // ESP32 RX <- G60 TX
constexpr int GPS_TX_PIN = 17;   // ESP32 TX -> G60 RX
constexpr uint32_t GPS_BAUD = 9600;


// =====================================================
// SD
// =====================================================

constexpr int SD_CS_PIN   = 10;
constexpr int SD_MOSI_PIN = 11;
constexpr int SD_MISO_PIN = 13;
constexpr int SD_SCK_PIN  = 12;

bool sdOK = false;


// =====================================================
// 固定起始时间
//
// 目标北京时间：2026-08-31 20:50:00
//
// 这里把 ESP32 系统时钟设置为对应的 UTC：
// 2026-08-31 12:50:00 UTC
//
// 再把 TZ 设置为 UTC+8，因此 localtime() 显示：
// 2026-08-31 20:50:00
//
// 重要：
// 每次重新上电或 RESET，都会重新从这个时间开始。
// GPS 不参与校时。
// =====================================================

constexpr time_t FIXED_START_EPOCH_UTC = 1788180600;

void initFixedClock()
{
    // POSIX TZ 规则中 CST-8 表示 UTC+8
    setenv("TZ", "CST-8", 1);
    tzset();

    timeval tv;
    tv.tv_sec = FIXED_START_EPOCH_UTC;
    tv.tv_usec = 0;

    settimeofday(&tv, nullptr);

    Serial.println(
        "[TIME] Fixed start = 2026-08-31 20:50:00"
    );
    Serial.println(
        "[TIME] GPS time disabled"
    );
}


String getDateTimeString()
{
    time_t now = time(nullptr);

    struct tm timeInfo;

    if (localtime_r(&now, &timeInfo) == nullptr)
    {
        return String("---- -- -- --:--:--");
    }

    char buffer[24];

    snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02d %02d:%02d:%02d",
        timeInfo.tm_year + 1900,
        timeInfo.tm_mon + 1,
        timeInfo.tm_mday,
        timeInfo.tm_hour,
        timeInfo.tm_min,
        timeInfo.tm_sec
    );

    return String(buffer);
}


// =====================================================
// 工作模式
// =====================================================

enum class WorkMode
{
    MODE_AUTO,
    MODE_HIGH,
    MODE_NORMAL,
    MODE_CALM
};

// 默认启动 AUTO。
// AUTO 启动后，在完成姿态基准校准前，实际工作状态先保持 NORMAL。
WorkMode controlMode = WorkMode::MODE_AUTO;
WorkMode autoState   = WorkMode::MODE_NORMAL;


// =====================================================
// 各模式完整数据采样周期
// =====================================================

constexpr uint32_t HIGH_INTERVAL_MS   = 1000;      // 1 秒
constexpr uint32_t NORMAL_INTERVAL_MS = 60000;     // 1 分钟
constexpr uint32_t CALM_INTERVAL_MS   = 600000;    // 10 分钟

// OLED 仍每秒刷新。
// 只刷新显示，不代表每秒都做完整采样。
constexpr uint32_t OLED_REFRESH_INTERVAL_MS = 1000;

// AUTO 模式下 MPU6050 每秒做一次轻量监测。
constexpr uint32_t AUTO_MONITOR_INTERVAL_MS = 1000;


// =====================================================
// AUTO 第一版工程测试阈值
//
// 注意：
// 这些是“样机调试初始阈值”，不是经过海试验证的正式海况标准。
// 后续应根据真实浮标采样数据重新标定。
// =====================================================

// ---------- 姿态偏移 ----------
// 相对于开机后的初始姿态基准。
constexpr float CALM_TILT_DELTA_DEG   = 3.0f;
constexpr float NORMAL_TILT_DELTA_DEG = 5.0f;
constexpr float HIGH_TILT_DELTA_DEG   = 15.0f;

// ---------- 最近约 10 秒姿态峰峰值 ----------
constexpr float NORMAL_RANGE_DEG = 5.0f;
constexpr float HIGH_RANGE_DEG   = 20.0f;

// ---------- 加速度模长相对 1g 的偏差 ----------
constexpr float NORMAL_ACCEL_DELTA_G = 0.08f;
constexpr float HIGH_ACCEL_DELTA_G   = 0.25f;

// ---------- 水温辅助阈值 ----------
constexpr float COLD_WATER_C     = -1.5f;
constexpr float FREEZING_WATER_C = -1.8f;

// 10 分钟内水温变化达到 1.0℃，暂时禁止进入 CALM。
constexpr float RAPID_TEMP_CHANGE_C = 1.0f;


// =====================================================
// AUTO 状态持续时间
// =====================================================

// HIGH 条件连续成立 10 秒 -> HIGH
constexpr uint64_t ENTER_HIGH_MS = 10ULL * 1000ULL;

// HIGH 条件消失并持续稳定 5 分钟 -> NORMAL
constexpr uint64_t EXIT_HIGH_MS = 5ULL * 60ULL * 1000ULL;

// NORMAL 持续非常稳定 30 分钟 -> CALM
constexpr uint64_t ENTER_CALM_MS = 30ULL * 60ULL * 1000ULL;

// CALM 中明显不再平静持续 10 秒 -> NORMAL
constexpr uint64_t EXIT_CALM_MS = 10ULL * 1000ULL;

// 水温 <= -1.5℃ 持续 5 分钟 -> 禁止 CALM，最低保持 NORMAL
constexpr uint64_t COLD_CONFIRM_MS = 5ULL * 60ULL * 1000ULL;

// 水温趋势参考窗口
constexpr uint64_t TEMP_TREND_WINDOW_MS = 10ULL * 60ULL * 1000ULL;

// 发生 >=1℃/10min 的变化后，10 分钟内不允许 CALM
constexpr uint64_t TEMP_GUARD_MS = 10ULL * 60ULL * 1000ULL;


// =====================================================
// AUTO 姿态基准与 10 秒窗口
// =====================================================

// 开机后 AUTO 用前 20 个有效 MPU6050 监测点计算初始姿态。
// AUTO 每秒监测一次，因此大约 20 秒完成。
constexpr uint16_t AUTO_CALIBRATION_SAMPLES = 20;

float baselineRollDeg  = 0.0f;
float baselinePitchDeg = 0.0f;
float baselineRollSum  = 0.0f;
float baselinePitchSum = 0.0f;

uint16_t baselineSampleCount = 0;
bool baselineReady = false;


// 10 秒窗口
constexpr uint8_t AUTO_WINDOW_SIZE = 10;

float rollWindow[AUTO_WINDOW_SIZE];
float pitchWindow[AUTO_WINDOW_SIZE];

uint8_t autoWindowCount = 0;
uint8_t autoWindowIndex = 0;


// 最近一次 AUTO 指标，便于串口查看
float lastAutoTiltOffsetDeg = 0.0f;
float lastAutoRangeDeg      = 0.0f;
float lastAutoAccelDeltaG   = 0.0f;


// =====================================================
// AUTO 计时状态
// =====================================================

uint64_t highConditionSinceMs = 0;
uint64_t highRecoverySinceMs  = 0;
uint64_t calmConditionSinceMs = 0;
uint64_t leaveCalmSinceMs     = 0;
uint64_t coldSinceMs          = 0;

uint64_t rapidTempGuardUntilMs = 0;


// =====================================================
// 水温趋势
// =====================================================

float tempReferenceC = NAN;
uint64_t tempReferenceMs = 0;


// =====================================================
// 调度
// =====================================================

uint64_t lastSampleMs      = 0;
uint64_t lastOledMs        = 0;
uint64_t lastAutoMonitorMs = 0;

bool firstSample = true;
bool forceFullSample = false;


// =====================================================
// Objects
// =====================================================

BuoySensors sensors;

// DS18B20 DATA -> GPIO4
DS18B20 waterSensor;

HardwareSerial G60Serial(1);
TinyGPSPlus gps;


// =====================================================
// Latest sample
// =====================================================

ImuData latestImu;
EnvData latestEnv;

float latestWaterTemp = NAN;
bool latestSampleReady = false;


// =====================================================
// BLE
// =====================================================

constexpr const char *BLE_DEVICE_NAME = "Marine_Buoy";

#define BLE_SERVICE_UUID        "12345678-1234-1234-1234-123456789000"
#define BLE_CHARACTERISTIC_UUID "12345678-1234-1234-1234-123456789001"

BLECharacteristic *bleCharacteristic = nullptr;
bool bleClientConnected = false;


// =====================================================
// Helper：当前单调运行时间
// =====================================================

uint64_t monotonicMs()
{
    return static_cast<uint64_t>(
        esp_timer_get_time() / 1000ULL
    );
}


// =====================================================
// 模式名称
// =====================================================

const char *modeName(WorkMode mode)
{
    switch (mode)
    {
        case WorkMode::MODE_AUTO:
            return "AUTO";

        case WorkMode::MODE_HIGH:
            return "HIGH";

        case WorkMode::MODE_NORMAL:
            return "NORMAL";

        case WorkMode::MODE_CALM:
            return "CALM";
    }

    return "UNKNOWN";
}


// =====================================================
// 实际工作状态
//
// controlMode=AUTO 时：
// 返回 AUTO 当前判断出来的 HIGH/NORMAL/CALM。
//
// 手动模式时：
// 直接返回用户指定的模式。
// =====================================================

WorkMode effectiveMode()
{
    if (controlMode == WorkMode::MODE_AUTO)
    {
        return autoState;
    }

    return controlMode;
}


// =====================================================
// 当前完整采样周期
// =====================================================

uint32_t currentSampleIntervalMs()
{
    switch (effectiveMode())
    {
        case WorkMode::MODE_HIGH:
            return HIGH_INTERVAL_MS;

        case WorkMode::MODE_CALM:
            return CALM_INTERVAL_MS;

        case WorkMode::MODE_NORMAL:
        case WorkMode::MODE_AUTO:
        default:
            return NORMAL_INTERVAL_MS;
    }
}


uint32_t currentSampleIntervalSec()
{
    return currentSampleIntervalMs() / 1000UL;
}


// =====================================================
// 重置 AUTO 状态计时器
// =====================================================

void resetAutoTransitionTimers()
{
    highConditionSinceMs = 0;
    highRecoverySinceMs  = 0;
    calmConditionSinceMs = 0;
    leaveCalmSinceMs     = 0;
}


// =====================================================
// 切换 AUTO 内部实际状态
// =====================================================

void setAutoState(
    WorkMode newState,
    const char *reason
)
{
    if (newState == WorkMode::MODE_AUTO)
    {
        return;
    }

    if (autoState == newState)
    {
        return;
    }

    Serial.print("[AUTO] State ");
    Serial.print(modeName(autoState));
    Serial.print(" -> ");
    Serial.print(modeName(newState));
    Serial.print(" | ");
    Serial.println(reason);

    autoState = newState;

    resetAutoTransitionTimers();

    // 模式变化后立刻做一次完整采样，
    // 不必等待新的 1 秒 / 1 分钟 / 10 分钟周期。
    forceFullSample = true;
}


// =====================================================
// 设置控制模式
//
// 可由 BLE WRITE 调用。
// =====================================================

void setControlMode(
    WorkMode newMode
)
{
    if (controlMode == newMode)
    {
        return;
    }

    Serial.print("[MODE] Control ");
    Serial.print(modeName(controlMode));
    Serial.print(" -> ");
    Serial.println(modeName(newMode));

    controlMode = newMode;

    if (controlMode == WorkMode::MODE_AUTO)
    {
        // 重新进入 AUTO 时从 NORMAL 开始判断，
        // 防止直接继承一个旧的 HIGH/CALM 状态。
        autoState = WorkMode::MODE_NORMAL;
        resetAutoTransitionTimers();

        Serial.println(
            "[AUTO] Effective state reset to NORMAL"
        );

        if (!baselineReady)
        {
            Serial.println(
                "[AUTO] Keep buoy relatively still while baseline is calibrating"
            );
        }
    }

    // 手动模式或重新进入 AUTO 后立即采一条。
    forceFullSample = true;
}


// =====================================================
// BLE Server callbacks
// =====================================================

class BuoyBleServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *server) override
    {
        bleClientConnected = true;

        Serial.println(
            "[BLE] Client connected"
        );
    }


    void onDisconnect(BLEServer *server) override
    {
        bleClientConnected = false;

        Serial.println(
            "[BLE] Client disconnected"
        );

        // 断开后重新广播
        BLEDevice::startAdvertising();
    }
};


// =====================================================
// BLE WRITE callback
//
// 手机写：
// AUTO
// HIGH
// NORMAL
// CALM
// =====================================================

class BuoyBleCommandCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(
        BLECharacteristic *characteristic
    ) override
    {
        String command =
            String(
                characteristic
                    ->getValue()
                    .c_str()
            );

        command.trim();
        command.toUpperCase();

        Serial.print(
            "[BLE] Command = "
        );
        Serial.println(command);

        if (command == "AUTO")
        {
            setControlMode(
                WorkMode::MODE_AUTO
            );
        }
        else if (command == "HIGH")
        {
            setControlMode(
                WorkMode::MODE_HIGH
            );
        }
        else if (command == "NORMAL")
        {
            setControlMode(
                WorkMode::MODE_NORMAL
            );
        }
        else if (command == "CALM")
        {
            setControlMode(
                WorkMode::MODE_CALM
            );
        }
        else
        {
            Serial.println(
                "[BLE] Unknown command. Use AUTO/HIGH/NORMAL/CALM"
            );
        }
    }
};


// =====================================================
// DS18B20 数据有效性
// =====================================================

bool isWaterTempValid(float temperature)
{
    return (
        isfinite(temperature) &&
        temperature > -55.0f &&
        temperature <= 125.0f
    );
}


// =====================================================
// 水温风险文字
// =====================================================

const char *waterTempRiskName()
{
    if (!isWaterTempValid(latestWaterTemp))
    {
        return "UNKNOWN";
    }

    if (latestWaterTemp <= FREEZING_WATER_C)
    {
        return "FREEZING";
    }

    if (latestWaterTemp <= COLD_WATER_C)
    {
        return "COLD";
    }

    return "NORMAL";
}


// =====================================================
// 水温是否已经持续低温并阻止 CALM
// =====================================================

bool coldBlocksCalm(
    uint64_t nowMs
)
{
    if (!isWaterTempValid(latestWaterTemp))
    {
        coldSinceMs = 0;
        return false;
    }

    if (latestWaterTemp <= COLD_WATER_C)
    {
        if (coldSinceMs == 0)
        {
            coldSinceMs = nowMs;
        }

        return (
            nowMs - coldSinceMs
            >=
            COLD_CONFIRM_MS
        );
    }

    coldSinceMs = 0;
    return false;
}


// =====================================================
// 水温快速变化保护
//
// 每次完整采样后调用。
// 比较大约 10 分钟前与当前水温。
// 如果变化 >= 1.0℃：
// 10 分钟内禁止进入 CALM。
// =====================================================

void updateTemperatureTrend(
    uint64_t nowMs
)
{
    if (!isWaterTempValid(latestWaterTemp))
    {
        return;
    }

    if (
        !isfinite(tempReferenceC) ||
        tempReferenceMs == 0
    )
    {
        tempReferenceC = latestWaterTemp;
        tempReferenceMs = nowMs;
        return;
    }

    if (
        nowMs - tempReferenceMs
        <
        TEMP_TREND_WINDOW_MS
    )
    {
        return;
    }

    float delta =
        fabsf(
            latestWaterTemp -
            tempReferenceC
        );

    if (
        delta
        >=
        RAPID_TEMP_CHANGE_C
    )
    {
        rapidTempGuardUntilMs =
            nowMs +
            TEMP_GUARD_MS;

        Serial.printf(
            "[AUTO] Water temperature changed %.2f C in ~10 min -> CALM blocked\n",
            delta
        );
    }

    tempReferenceC = latestWaterTemp;
    tempReferenceMs = nowMs;
}


bool rapidTemperatureBlocksCalm(
    uint64_t nowMs
)
{
    return (
        rapidTempGuardUntilMs != 0 &&
        nowMs < rapidTempGuardUntilMs
    );
}


// =====================================================
// AUTO 10 秒姿态窗口
// =====================================================

void resetAutoWindow()
{
    autoWindowCount = 0;
    autoWindowIndex = 0;
}


void pushAutoWindow(
    float roll,
    float pitch
)
{
    rollWindow[autoWindowIndex] = roll;
    pitchWindow[autoWindowIndex] = pitch;

    autoWindowIndex =
        (autoWindowIndex + 1)
        %
        AUTO_WINDOW_SIZE;

    if (
        autoWindowCount
        <
        AUTO_WINDOW_SIZE
    )
    {
        autoWindowCount++;
    }
}


float getAutoRollRange()
{
    if (autoWindowCount == 0)
    {
        return 0.0f;
    }

    float minValue = rollWindow[0];
    float maxValue = rollWindow[0];

    for (
        uint8_t i = 1;
        i < autoWindowCount;
        i++
    )
    {
        minValue =
            min(
                minValue,
                rollWindow[i]
            );

        maxValue =
            max(
                maxValue,
                rollWindow[i]
            );
    }

    return maxValue - minValue;
}


float getAutoPitchRange()
{
    if (autoWindowCount == 0)
    {
        return 0.0f;
    }

    float minValue = pitchWindow[0];
    float maxValue = pitchWindow[0];

    for (
        uint8_t i = 1;
        i < autoWindowCount;
        i++
    )
    {
        minValue =
            min(
                minValue,
                pitchWindow[i]
            );

        maxValue =
            max(
                maxValue,
                pitchWindow[i]
            );
    }

    return maxValue - minValue;
}


// =====================================================
// AUTO 初始姿态校准
// =====================================================

void updateAutoBaseline(
    const ImuData &imu
)
{
    if (
        baselineReady ||
        !imu.valid
    )
    {
        return;
    }

    baselineRollSum +=
        imu.rollDeg;

    baselinePitchSum +=
        imu.pitchDeg;

    baselineSampleCount++;

    Serial.printf(
        "[AUTO] Baseline calibration %u/%u\n",
        baselineSampleCount,
        AUTO_CALIBRATION_SAMPLES
    );

    if (
        baselineSampleCount
        >=
        AUTO_CALIBRATION_SAMPLES
    )
    {
        baselineRollDeg =
            baselineRollSum /
            baselineSampleCount;

        baselinePitchDeg =
            baselinePitchSum /
            baselineSampleCount;

        baselineReady = true;

        resetAutoWindow();

        Serial.printf(
            "[AUTO] Baseline ready | Roll=%.2f deg | Pitch=%.2f deg\n",
            baselineRollDeg,
            baselinePitchDeg
        );
    }
}


// =====================================================
// AUTO 判断
//
// HIGH：
// 1) 相对初始姿态偏移 >= 15°
// 或 2) 最近约10秒姿态峰峰值 >= 20°
// 或 3) 加速度模长相对1g偏差 >= 0.25g
//
// CALM 候选：
// 1) 姿态偏移 < 3°
// 且 2) 最近约10秒峰峰值 < 5°
// 且 3) 加速度偏差 < 0.08g
// 且 4) 无持续低温/快速温变保护
//
// 其余作为 NORMAL 区域。
// =====================================================

void processAutoImu(
    const ImuData &imu,
    uint64_t nowMs
)
{
    if (
        controlMode != WorkMode::MODE_AUTO ||
        !imu.valid
    )
    {
        return;
    }

    if (!baselineReady)
    {
        updateAutoBaseline(imu);
        return;
    }

    pushAutoWindow(
        imu.rollDeg,
        imu.pitchDeg
    );


    // ---------- 相对初始姿态偏移 ----------

    float rollOffset =
        fabsf(
            imu.rollDeg -
            baselineRollDeg
        );

    float pitchOffset =
        fabsf(
            imu.pitchDeg -
            baselinePitchDeg
        );

    float maxTiltOffset =
        max(
            rollOffset,
            pitchOffset
        );


    // ---------- 最近约10秒姿态峰峰值 ----------

    float rollRange =
        getAutoRollRange();

    float pitchRange =
        getAutoPitchRange();

    float maxRange =
        max(
            rollRange,
            pitchRange
        );


    // ---------- 加速度模长相对 1g 偏差 ----------

    float accelMagnitude =
        sqrtf(
            imu.ax * imu.ax +
            imu.ay * imu.ay +
            imu.az * imu.az
        );

    float accelG =
        accelMagnitude /
        BuoyConfig::STANDARD_GRAVITY;

    float accelDeltaG =
        fabsf(
            accelG -
            1.0f
        );


    // 保存用于串口诊断
    lastAutoTiltOffsetDeg =
        maxTiltOffset;

    lastAutoRangeDeg =
        maxRange;

    lastAutoAccelDeltaG =
        accelDeltaG;


    // ---------- 条件 ----------

    bool highCondition =
        (
            maxTiltOffset
            >=
            HIGH_TILT_DELTA_DEG
        )
        ||
        (
            maxRange
            >=
            HIGH_RANGE_DEG
        )
        ||
        (
            accelDeltaG
            >=
            HIGH_ACCEL_DELTA_G
        );


    bool calmMotionCondition =
        (
            maxTiltOffset
            <
            CALM_TILT_DELTA_DEG
        )
        &&
        (
            maxRange
            <
            NORMAL_RANGE_DEG
        )
        &&
        (
            accelDeltaG
            <
            NORMAL_ACCEL_DELTA_G
        );


    bool temperatureBlocksCalm =
        coldBlocksCalm(nowMs)
        ||
        rapidTemperatureBlocksCalm(
            nowMs
        );


    // =================================================
    // 1. HIGH 优先级最高
    // =================================================

    if (highCondition)
    {
        highRecoverySinceMs = 0;
        calmConditionSinceMs = 0;
        leaveCalmSinceMs = 0;

        if (highConditionSinceMs == 0)
        {
            highConditionSinceMs =
                nowMs;
        }

        if (
            autoState != WorkMode::MODE_HIGH &&
            nowMs - highConditionSinceMs
            >=
            ENTER_HIGH_MS
        )
        {
            setAutoState(
                WorkMode::MODE_HIGH,
                "high-motion condition sustained for 10 s"
            );

            return;
        }
    }
    else
    {
        highConditionSinceMs = 0;
    }


    // =================================================
    // 2. 当前处于 HIGH：稳定 5 分钟才能降到 NORMAL
    // =================================================

    if (
        autoState ==
        WorkMode::MODE_HIGH
    )
    {
        if (!highCondition)
        {
            if (
                highRecoverySinceMs
                ==
                0
            )
            {
                highRecoverySinceMs =
                    nowMs;
            }

            if (
                nowMs -
                highRecoverySinceMs
                >=
                EXIT_HIGH_MS
            )
            {
                setAutoState(
                    WorkMode::MODE_NORMAL,
                    "high condition cleared for 5 min"
                );
            }
        }
        else
        {
            highRecoverySinceMs = 0;
        }

        return;
    }


    // =================================================
    // 3. 当前处于 NORMAL：非常稳定 30 分钟 -> CALM
    // =================================================

    if (
        autoState ==
        WorkMode::MODE_NORMAL
    )
    {
        if (
            calmMotionCondition &&
            !temperatureBlocksCalm
        )
        {
            if (
                calmConditionSinceMs
                ==
                0
            )
            {
                calmConditionSinceMs =
                    nowMs;
            }

            if (
                nowMs -
                calmConditionSinceMs
                >=
                ENTER_CALM_MS
            )
            {
                setAutoState(
                    WorkMode::MODE_CALM,
                    "very stable for 30 min"
                );
            }
        }
        else
        {
            calmConditionSinceMs = 0;
        }

        return;
    }


    // =================================================
    // 4. 当前处于 CALM：
    //    不再满足平静条件或温度保护触发，持续 10 秒 -> NORMAL
    // =================================================

    if (
        autoState ==
        WorkMode::MODE_CALM
    )
    {
        bool leaveCalm =
            !calmMotionCondition
            ||
            temperatureBlocksCalm;

        if (leaveCalm)
        {
            if (
                leaveCalmSinceMs
                ==
                0
            )
            {
                leaveCalmSinceMs =
                    nowMs;
            }

            if (
                nowMs -
                leaveCalmSinceMs
                >=
                EXIT_CALM_MS
            )
            {
                setAutoState(
                    WorkMode::MODE_NORMAL,
                    temperatureBlocksCalm
                        ? "temperature guard"
                        : "motion no longer calm"
                );
            }
        }
        else
        {
            leaveCalmSinceMs = 0;
        }
    }
}


// =====================================================
// AUTO 每秒轻量监测 MPU6050
//
// 注意：
// CALM 虽然完整数据 10 分钟保存一次，
// 但 AUTO 仍每秒读取 MPU6050 做状态判断。
// 这次轻量读取不写 SD、不发 BLE。
// =====================================================

void serviceAutoMonitor(
    uint64_t nowMs
)
{
    if (
        controlMode != WorkMode::MODE_AUTO
    )
    {
        return;
    }

    if (
        nowMs -
        lastAutoMonitorMs
        <
        AUTO_MONITOR_INTERVAL_MS
    )
    {
        return;
    }

    lastAutoMonitorMs =
        nowMs;

    ImuData monitorImu;

    sensors.readImu(
        monitorImu
    );

    processAutoImu(
        monitorImu,
        nowMs
    );
}


// =====================================================
// SD 初始化
// =====================================================

void initSD()
{
    Serial.println(
        "[SD] Initializing..."
    );

    SPI.begin(
        SD_SCK_PIN,
        SD_MISO_PIN,
        SD_MOSI_PIN,
        SD_CS_PIN
    );

    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println(
            "[SD] Card mount failed"
        );

        sdOK = false;
        return;
    }

    sdOK = true;

    Serial.println(
        "[SD] Card OK"
    );

    File file =
        SD.open(
            "/buoy.csv",
            FILE_APPEND
        );

    if (file)
    {
        if (
            file.size() == 0
        )
        {
            file.println(
                "time,lat,lon,temp,pressure,humidity,water_temp,roll,pitch,control_mode,work_state,sample_interval_s,temp_risk"
            );
        }

        file.close();
    }
}


// =====================================================
// GPS 持续解析
//
// GPS 只做定位。
// 不再读取 GPS 日期/时间。
// =====================================================

void serviceGps()
{
    while (
        G60Serial.available()
    )
    {
        gps.encode(
            G60Serial.read()
        );
    }
}


// =====================================================
// BLE 初始化
// =====================================================

void initBLE()
{
    Serial.println(
        "[BLE] Initializing..."
    );

    BLEDevice::init(
        BLE_DEVICE_NAME
    );

    // 尝试使用较大 MTU
    BLEDevice::setMTU(247);

    BLEServer *server =
        BLEDevice::createServer();

    server->setCallbacks(
        new BuoyBleServerCallbacks()
    );

    BLEService *service =
        server->createService(
            BLE_SERVICE_UUID
        );

    bleCharacteristic =
        service->createCharacteristic(
            BLE_CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_WRITE
        );

    bleCharacteristic->setCallbacks(
        new BuoyBleCommandCallbacks()
    );

    bleCharacteristic->addDescriptor(
        new BLE2902()
    );

    bleCharacteristic->setValue(
        "Marine Buoy BLE Ready"
    );

    service->start();

    BLEAdvertising *advertising =
        BLEDevice::getAdvertising();

    advertising->addServiceUUID(
        BLE_SERVICE_UUID
    );

    advertising->setScanResponse(
        true
    );

    BLEDevice::startAdvertising();

    Serial.println(
        "[BLE] Marine_Buoy started"
    );

    Serial.println(
        "[BLE] WRITE commands: AUTO / HIGH / NORMAL / CALM"
    );
}


// =====================================================
// SD 保存
// =====================================================

void saveData(
    const ImuData &imu,
    const EnvData &env,
    float waterTemp
)
{
    if (!sdOK)
    {
        return;
    }

    File file =
        SD.open(
            "/buoy.csv",
            FILE_APPEND
        );

    if (!file)
    {
        return;
    }


    // ---------- 时间 ----------

    file.print(
        getDateTimeString()
    );

    file.print(",");


    // ---------- GPS ----------

    if (
        gps.location.isValid() &&
        gps.location.age() < 5000
    )
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
        file.print(
            "0,0"
        );
    }


    // ---------- BME280 温度 ----------

    file.print(",");

    if (
        env.valid &&
        isfinite(
            env.temperatureC
        )
    )
    {
        file.print(
            env.temperatureC,
            2
        );
    }


    // ---------- BME280 气压 ----------

    file.print(",");

    if (
        env.valid &&
        isfinite(
            env.pressureHpa
        )
    )
    {
        file.print(
            env.pressureHpa,
            2
        );
    }


    // ---------- BME280 湿度 ----------

    file.print(",");

    if (
        env.valid &&
        env.hasHumidity &&
        isfinite(
            env.humidityPct
        )
    )
    {
        file.print(
            env.humidityPct,
            1
        );
    }


    // ---------- DS18B20 水温 ----------

    file.print(",");

    if (
        isWaterTempValid(
            waterTemp
        )
    )
    {
        file.print(
            waterTemp,
            2
        );
    }


    // ---------- Roll ----------

    file.print(",");

    if (imu.valid)
    {
        file.print(
            imu.rollDeg,
            2
        );
    }


    // ---------- Pitch ----------

    file.print(",");

    if (imu.valid)
    {
        file.print(
            imu.pitchDeg,
            2
        );
    }


    // ---------- 控制模式 ----------

    file.print(",");
    file.print(
        modeName(
            controlMode
        )
    );


    // ---------- 实际工作状态 ----------

    file.print(",");
    file.print(
        modeName(
            effectiveMode()
        )
    );


    // ---------- 采样周期 ----------

    file.print(",");
    file.print(
        currentSampleIntervalSec()
    );


    // ---------- 温度风险 ----------

    file.print(",");
    file.print(
        waterTempRiskName()
    );


    file.println();

    file.close();
}


// =====================================================
// BLE 发送
// =====================================================

void sendBleData(
    const ImuData &imu,
    const EnvData &env,
    float waterTemp
)
{
    if (
        bleCharacteristic
        ==
        nullptr
    )
    {
        return;
    }

    String data;

    data.reserve(220);

    data +=
        getDateTimeString();


    data += ",T=";

    if (
        env.valid &&
        isfinite(
            env.temperatureC
        )
    )
    {
        data += String(
            env.temperatureC,
            2
        );
    }
    else
    {
        data += "NA";
    }


    data += ",P=";

    if (
        env.valid &&
        isfinite(
            env.pressureHpa
        )
    )
    {
        data += String(
            env.pressureHpa,
            2
        );
    }
    else
    {
        data += "NA";
    }


    data += ",H=";

    if (
        env.valid &&
        env.hasHumidity &&
        isfinite(
            env.humidityPct
        )
    )
    {
        data += String(
            env.humidityPct,
            1
        );
    }
    else
    {
        data += "NA";
    }


    data += ",WT=";

    if (
        isWaterTempValid(
            waterTemp
        )
    )
    {
        data += String(
            waterTemp,
            2
        );
    }
    else
    {
        data += "NA";
    }


    data += ",R=";

    if (imu.valid)
    {
        data += String(
            imu.rollDeg,
            2
        );
    }
    else
    {
        data += "NA";
    }


    data += ",Pi=";

    if (imu.valid)
    {
        data += String(
            imu.pitchDeg,
            2
        );
    }
    else
    {
        data += "NA";
    }


    data += ",Lat=";

    if (
        gps.location.isValid() &&
        gps.location.age() < 5000
    )
    {
        data += String(
            gps.location.lat(),
            6
        );
    }
    else
    {
        data += "NA";
    }


    data += ",Lon=";

    if (
        gps.location.isValid() &&
        gps.location.age() < 5000
    )
    {
        data += String(
            gps.location.lng(),
            6
        );
    }
    else
    {
        data += "NA";
    }


    data += ",M=";
    data += modeName(
        controlMode
    );

    data += ",S=";
    data += modeName(
        effectiveMode()
    );

    data += ",I=";
    data += String(
        currentSampleIntervalSec()
    );

    data += ",SD=";
    data +=
        sdOK
            ? "OK"
            : "ERR";


    bleCharacteristic->setValue(
        data.c_str()
    );

    if (
        bleClientConnected
    )
    {
        bleCharacteristic->notify();
    }
}


// =====================================================
// 串口显示
// =====================================================

void printSerial(
    const ImuData &imu,
    const EnvData &env,
    float waterTemp
)
{
    Serial.println();

    Serial.println(
        "========== 浮标实时数据 =========="
    );


    // ---------- 时间 ----------

    Serial.print(
        "时间   "
    );

    Serial.println(
        getDateTimeString()
    );


    // ---------- 模式 ----------

    Serial.printf(
        "模式   控制=%s | 状态=%s | 完整采样=%lu秒/次\n",
        modeName(
            controlMode
        ),
        modeName(
            effectiveMode()
        ),
        static_cast<unsigned long>(
            currentSampleIntervalSec()
        )
    );


    // ---------- 环境 ----------

    if (env.valid)
    {
        Serial.printf(
            "环境   温度 %.2f C | 气压 %.2f hPa",
            env.temperatureC,
            env.pressureHpa
        );

        if (
            env.hasHumidity &&
            isfinite(
                env.humidityPct
            )
        )
        {
            Serial.printf(
                " | 湿度 %.1f %%",
                env.humidityPct
            );
        }

        Serial.println();
    }
    else
    {
        Serial.println(
            "环境   数据异常"
        );
    }


    // ---------- 水温 ----------

    if (
        isWaterTempValid(
            waterTemp
        )
    )
    {
        Serial.printf(
            "水温   %.2f C | 温度风险 %s\n",
            waterTemp,
            waterTempRiskName()
        );
    }
    else
    {
        Serial.println(
            "水温   DS18B20传感器异常"
        );
    }


    // ---------- 姿态 ----------

    if (imu.valid)
    {
        Serial.printf(
            "姿态   横滚 %+6.2f | 俯仰 %+6.2f\n",
            imu.rollDeg,
            imu.pitchDeg
        );

        Serial.printf(
            "加速度 X %+5.2f | Y %+5.2f | Z %+5.2f m/s2\n",
            imu.ax,
            imu.ay,
            imu.az
        );
    }
    else
    {
        Serial.println(
            "姿态   数据异常"
        );

        Serial.println(
            "加速度 数据异常"
        );
    }


    // ---------- AUTO ----------

    if (
        controlMode ==
        WorkMode::MODE_AUTO
    )
    {
        if (baselineReady)
        {
            Serial.printf(
                "AUTO   基准R %.2f P %.2f | 偏移 %.2f° | 10秒波动 %.2f° | Δa %.3fg\n",
                baselineRollDeg,
                baselinePitchDeg,
                lastAutoTiltOffsetDeg,
                lastAutoRangeDeg,
                lastAutoAccelDeltaG
            );
        }
        else
        {
            Serial.printf(
                "AUTO   姿态基准校准中 %u/%u\n",
                baselineSampleCount,
                AUTO_CALIBRATION_SAMPLES
            );
        }
    }


    // ---------- GPS ----------

    Serial.print(
        "定位   "
    );

    if (
        gps.location.isValid() &&
        gps.location.age() < 5000
    )
    {
        Serial.printf(
            "已定位 | %.6f, %.6f",
            gps.location.lat(),
            gps.location.lng()
        );

        if (
            gps.satellites.isValid()
        )
        {
            Serial.printf(
                " | 卫星 %lu颗",
                gps.satellites.value()
            );
        }

        Serial.println();
    }
    else if (
        gps.charsProcessed() > 10
    )
    {
        Serial.println(
            "搜索卫星中..."
        );
    }
    else
    {
        Serial.println(
            "暂无GPS数据"
        );
    }


    // ---------- SD ----------

    Serial.print(
        "存储   "
    );

    if (sdOK)
    {
        Serial.println(
            "SD卡正常"
        );
    }
    else
    {
        Serial.println(
            "SD卡异常"
        );
    }


    // ---------- BLE ----------

    Serial.print(
        "蓝牙   "
    );

    if (
        bleClientConnected
    )
    {
        Serial.println(
            "已连接"
        );
    }
    else
    {
        Serial.println(
            "等待连接"
        );
    }


    Serial.println(
        "================================="
    );
}


// =====================================================
// 执行一次完整传感器采样
// =====================================================

void sampleSensors(
    uint64_t nowMs
)
{
    sensors.readImu(
        latestImu
    );

    sensors.readEnvironment(
        latestEnv
    );

    latestWaterTemp =
        waterSensor.getTemperature();

    latestSampleReady =
        true;


    // 更新水温 10 分钟变化趋势
    updateTemperatureTrend(
        nowMs
    );


    // 串口
    printSerial(
        latestImu,
        latestEnv,
        latestWaterTemp
    );


    // SD
    saveData(
        latestImu,
        latestEnv,
        latestWaterTemp
    );


    // BLE
    sendBleData(
        latestImu,
        latestEnv,
        latestWaterTemp
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

    Serial.println();
    Serial.println(
        "BUOY V1.6 START"
    );


    // =================================================
    // I2C
    // MPU6050 + BME280 + OLED
    // =================================================

    Wire.begin(
        BuoyConfig::I2C_SDA_PIN,
        BuoyConfig::I2C_SCL_PIN,
        BuoyConfig::I2C_FREQUENCY_HZ
    );

    I2CUtils::scanBus(
        Wire,
        Serial
    );


    // =================================================
    // MPU6050 + BME280
    // =================================================

    sensors.begin(
        Wire,
        Serial
    );


    // =================================================
    // DS18B20
    // DATA -> GPIO4
    // =================================================

    waterSensor.begin();

    Serial.println(
        "[DS18B20] Water temperature sensor started"
    );


    // =================================================
    // OLED
    // =================================================

    oledBegin();


    // =================================================
    // SD
    // =================================================

    initSD();


    // =================================================
    // GPS
    //
    // 只用于定位。
    // GPS 日期和时间不参与系统时间。
    // =================================================

    G60Serial.begin(
        GPS_BAUD,
        SERIAL_8N1,
        GPS_RX_PIN,
        GPS_TX_PIN
    );

    Serial.println(
        "[G60] UART started | positioning only"
    );


    // =================================================
    // BLE
    // =================================================

    initBLE();


    // =================================================
    // 启动固定时间
    //
    // 放在所有初始化之后，
    // 这样 loop() 第一条完整数据尽量就是：
    // 2026-08-31 20:50:00
    // =================================================

    initFixedClock();


    Serial.println(
        "[READY] Control mode = AUTO"
    );

    Serial.println(
        "[READY] AUTO initial effective state = NORMAL"
    );

    Serial.println(
        "[READY] HIGH=1s | NORMAL=60s | CALM=600s"
    );

    Serial.println(
        "[READY] AUTO MPU monitor = 1 second"
    );

    Serial.println(
        "[READY] OLED refresh = 1 second"
    );

    Serial.println(
        "[READY] GPS parser = continuous, GPS time ignored"
    );

    Serial.println(
        "[AUTO] First ~20 s: keep buoy relatively still for baseline calibration"
    );
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    // -------------------------------------------------
    // GPS 持续读取
    // -------------------------------------------------

    serviceGps();


    uint64_t nowMs =
        monotonicMs();


    // -------------------------------------------------
    // AUTO 每秒轻量读取 MPU6050
    //
    // 即使 CALM 完整采样是 10 分钟一次，
    // AUTO 也不会等 10 分钟才发现运动异常。
    // -------------------------------------------------

    serviceAutoMonitor(
        nowMs
    );


    // -------------------------------------------------
    // 完整传感器采样
    //
    // HIGH   1 秒
    // NORMAL 1 分钟
    // CALM   10 分钟
    //
    // 开机后立即采第一条。
    // 模式发生变化时也立即采一条。
    // -------------------------------------------------

    uint32_t sampleIntervalMs =
        currentSampleIntervalMs();

    if (
        firstSample ||
        forceFullSample ||
        (
            nowMs -
            lastSampleMs
            >=
            sampleIntervalMs
        )
    )
    {
        firstSample = false;
        forceFullSample = false;

        lastSampleMs =
            nowMs;

        sampleSensors(
            nowMs
        );
    }


    // -------------------------------------------------
    // OLED 每秒刷新
    //
    // 当前 oled_display.cpp 如果读取 ESP32 系统时钟，
    // 会显示本 main.cpp 初始化的固定起始北京时间。
    //
    // 传感器数值继续显示最近一次完整采样结果。
    // -------------------------------------------------

    if (
        latestSampleReady &&
        (
            nowMs -
            lastOledMs
            >=
            OLED_REFRESH_INTERVAL_MS
        )
    )
    {
        lastOledMs =
            nowMs;

        oledUpdate(
            latestImu,
            latestEnv,
            gps,
            sdOK,
            latestWaterTemp,
            modeName(controlMode),
            modeName(effectiveMode()),
            currentSampleIntervalSec()
        );
    }


    delay(10);
}
