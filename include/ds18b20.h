#ifndef DS18B20_H
#define DS18B20_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>


// ============================================================
// DS18B20 防水温度传感器
// DATA → GPIO4
// ============================================================

constexpr uint8_t DS18B20_PIN = 4;


class DS18B20
{
private:

    OneWire oneWire_;
    DallasTemperature sensor_;

public:

    DS18B20()
        : oneWire_(DS18B20_PIN),
          sensor_(&oneWire_)
    {
    }


    void begin()
    {
        sensor_.begin();
    }


    float getTemperature()
    {
        sensor_.requestTemperatures();

        return sensor_.getTempCByIndex(0);
    }
};


#endif