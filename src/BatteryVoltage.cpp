#include "BatteryVoltage.h"
#include "Config.h"

BatteryVoltage batteryVoltage;

static const unsigned long kSampleIntervalMs = 80;
static const uint8_t kWarmupSamples = 6;
static const float kEmaAlpha = 0.12f;
static const bool kUseMockBatteryVoltage = true;
static const float kMockBatteryVoltage = 14.2f;

void BatteryVoltage::init() {
    pinMode(PIN_BATTERY_ADC, INPUT);
    analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db);
    filteredVolts = 0.f;
    warmupSum = 0.f;
    warmupCount = 0;
    haveReading = false;
    lastSampleMs = 0;
}

void BatteryVoltage::update() {
    unsigned long now = millis();
    if (now - lastSampleMs < kSampleIntervalMs) {
        return;
    }
    lastSampleMs = now;

    if (kUseMockBatteryVoltage) {
        filteredVolts = kMockBatteryVoltage;
        haveReading = true;
        warmupSum = 0.f;
        warmupCount = kWarmupSamples;
        return;
    }

    int pinMv = analogReadMilliVolts(PIN_BATTERY_ADC);
    if (pinMv < 0) {
        pinMv = 0;
    }
    float vBat = (pinMv / 1000.0f) * BATTERY_VOLTAGE_MULTIPLIER;

    if (warmupCount < kWarmupSamples) {
        warmupSum += vBat;
        warmupCount++;
        if (warmupCount >= kWarmupSamples) {
            filteredVolts = warmupSum / static_cast<float>(kWarmupSamples);
            haveReading = true;
        }
        return;
    }

    filteredVolts = filteredVolts * (1.0f - kEmaAlpha) + vBat * kEmaAlpha;
}

bool BatteryVoltage::isLowForRemoteStart() const {
    if (!haveReading) {
        return true;
    }
    return filteredVolts < BATTERY_LOW_START_BLOCK_V;
}

bool BatteryVoltage::isAlternatorCharging() const {
    if (!haveReading) {
        return false;
    }
    return filteredVolts >= BATTERY_ALTERNATOR_MIN_V;
}
