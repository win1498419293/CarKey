#pragma once
#include <Arduino.h>

// SleepManager - Low-power light sleep for vehicle battery preservation
// When idle (engine off, no auth, no web), ESP32 enters light sleep (~0.8mA)
// and wakes every 30s via RTC timer to check for remote commands.
// Wake time <1ms, RAM state preserved.

class SleepManager {
public:
    static void init();
    static void update();
    static bool isAwake();
    static unsigned long getSleepRemainingSec();

    static constexpr unsigned long kIdleTimeoutMs = 15000;
    static constexpr unsigned long kSleepDurationMs = 300000;
};

extern SleepManager sleepManager;
