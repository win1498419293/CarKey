#pragma once
#include <Arduino.h>

// ===== Engine Status (voltage-based detection with hysteresis) =====
struct EngineStatus {
    bool running = false;
    float batteryVoltage = 0.0f;
};

// ===== Battery Health Levels =====
enum class BatteryHealth : uint8_t {
    GOOD = 0,
    BAT_LOW,
    CRITICAL
};

// ===== Vehicle Status Manager =====
// Central module for all vehicle state detection:
//   - Engine running (voltage hysteresis)
//   - ACC state (GPIO)
//   - Hand brake (GPIO)
//   - Driver door (GPIO)
//   - Battery health
//   - Start result confirmation
class VehicleStatusManager {
public:
    void init();
    void update();

    // --- State Getters ---
    bool isEngineRunning() const { return engineStatus.running; }
    float getBatteryVoltage() const { return engineStatus.batteryVoltage; }
    const char* getBatteryHealthStr() const;
    bool isAccOn() const { return accOn; }
    bool isHandBrakeEngaged() const { return handBrakeEngaged; }
    bool isDriverDoorOpen() const { return driverDoorOpen; }

    // --- Start confirmation ---
    // Returns true if engine started successfully (voltage > 13.5V after delay)
    bool verifyEngineStart();

    // --- JSON for API ---
    String toJson() const;

private:
    // Engine detection via voltage hysteresis
    void updateEngineState();

    // GPIO state polling
    void updateGpioStates();

    // State change logging
    void logStateChanges();

    // Engine hysteresis constants
    static constexpr float kEngineRunningThreshold  = 13.5f;
    static constexpr float kEngineStoppedThreshold  = 13.2f;

    // Start confirmation
    static constexpr unsigned long kStartVerifyDelayMs = 3000;

    // Cached states for change detection logging
    EngineStatus engineStatus;
    bool accOn = false;
    bool handBrakeEngaged = false;
    bool driverDoorOpen = false;

    // Previous states for change detection
    bool prevEngineRunning = false;
    bool prevAccOn = false;
    bool prevHandBrake = false;
    bool prevDoorOpen = false;
    float prevVoltage = 0.0f;
    BatteryHealth prevBatteryHealth = BatteryHealth::GOOD;
};

extern VehicleStatusManager vehicleStatus;
