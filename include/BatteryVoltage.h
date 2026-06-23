#pragma once
#include <Arduino.h>

// Battery voltage ADC monitoring + low voltage alert (< 11.8V block remote start)
class BatteryVoltage {
public:
    void init();
    void update();
    float getVoltage() const { return filteredVolts; }
    bool hasReading() const { return haveReading; }
    bool isLowForRemoteStart() const;
    bool isAlternatorCharging() const;

private:
    float filteredVolts = 0.f;
    float warmupSum = 0.f;
    uint8_t warmupCount = 0;
    bool haveReading = false;
    unsigned long lastSampleMs = 0;
};

extern BatteryVoltage batteryVoltage;
