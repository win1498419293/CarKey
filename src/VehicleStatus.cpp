#include "VehicleStatus.h"
#include "Config.h"
#include "BatteryVoltage.h"
#include "Logger.h"
#include "RelayManager.h"

VehicleStatusManager vehicleStatus;

extern BatteryVoltage batteryVoltage;
extern RelayManager relayManager;

// Voltage sampling interval for engine detection (ms)
static const unsigned long kEngineSampleIntervalMs = 500;

static unsigned long gLastEngineSampleMs = 0;

// --- Helpers ---

static const char* batteryHealthToStr(BatteryHealth h) {
    switch (h) {
        case BatteryHealth::GOOD:     return "GOOD";
        case BatteryHealth::BAT_LOW:      return "LOW";
        case BatteryHealth::CRITICAL: return "CRITICAL";
        default:                      return "UNKNOWN";
    }
}

static BatteryHealth classifyBatteryHealth(float voltage) {
    if (voltage >= 12.5f) return BatteryHealth::GOOD;
    if (voltage >= 12.2f) return BatteryHealth::BAT_LOW;
    return BatteryHealth::CRITICAL;
}

// --- VehicleStatusManager ---

void VehicleStatusManager::init() {
    // ACC: GPIO32 input with pull-down (active HIGH = ACC ON)
    pinMode(PIN_ACC, INPUT_PULLDOWN);

    // Hand brake: GPIO34 input with pull-up (LOW = engaged, per existing convention)
    pinMode(PIN_HANDBRAKE, INPUT_PULLUP);

    // Driver door: GPIO35 input with pull-up (LOW = door open, per door switch convention)
    pinMode(PIN_DOOR, INPUT_PULLUP);

    // Reset all states
    engineStatus.running = false;
    engineStatus.batteryVoltage = 0.0f;
    accOn = false;
    handBrakeEngaged = false;
    driverDoorOpen = false;

    prevEngineRunning = false;
    prevAccOn = false;
    prevHandBrake = false;
    prevDoorOpen = false;
    prevVoltage = 0.0f;
    prevBatteryHealth = BatteryHealth::GOOD;

    gLastEngineSampleMs = 0;

    Logger::info("[VehicleStatus] Initialized");
}

void VehicleStatusManager::update() {
    updateGpioStates();
    updateEngineState();
    logStateChanges();
}

void VehicleStatusManager::updateGpioStates() {
    // ACC: GPIO32, HIGH = ON
    accOn = (digitalRead(PIN_ACC) == HIGH);

    // Hand brake: GPIO34, LOW = engaged (pull-up, switch to GND when engaged)
    handBrakeEngaged = (digitalRead(PIN_HANDBRAKE) == LOW);

    // Driver door: GPIO35, LOW = open (pull-up, switch to GND when door open)
    driverDoorOpen = (digitalRead(PIN_DOOR) == LOW);
}

void VehicleStatusManager::updateEngineState() {
    unsigned long now = millis();
    if (now - gLastEngineSampleMs < kEngineSampleIntervalMs) {
        return;
    }
    gLastEngineSampleMs = now;

    if (!batteryVoltage.hasReading()) {
        return;
    }

    float voltage = batteryVoltage.getVoltage();
    engineStatus.batteryVoltage = voltage;

    // Hysteresis-based engine detection:
    //   voltage > 13.5V  → engine running
    //   voltage < 13.2V  → engine stopped
    //   13.2V ~ 13.5V    → keep previous state
    if (voltage > kEngineRunningThreshold) {
        engineStatus.running = true;
    } else if (voltage < kEngineStoppedThreshold) {
        engineStatus.running = false;
    }
    // else: keep current state (hysteresis zone)
}

void VehicleStatusManager::logStateChanges() {
    // Engine state change
    if (engineStatus.running != prevEngineRunning) {
        if (engineStatus.running) {
            Logger::info("[ENGINE] Running");
        } else {
            Logger::info("[ENGINE] Stopped");
        }
        prevEngineRunning = engineStatus.running;
    }

    // Battery voltage significant change (>0.2V delta)
    if (abs(engineStatus.batteryVoltage - prevVoltage) > 0.2f) {
        Logger::info("[BATTERY] Voltage=" + String(engineStatus.batteryVoltage, 1) + "V");
        prevVoltage = engineStatus.batteryVoltage;
    }

    // Battery health change
    BatteryHealth currentHealth = classifyBatteryHealth(engineStatus.batteryVoltage);
    if (currentHealth != prevBatteryHealth) {
        Logger::info("[BATTERY] Health=" + String(batteryHealthToStr(currentHealth)));
        prevBatteryHealth = currentHealth;
    }

    // ACC state change
    if (accOn != prevAccOn) {
        Logger::info("[ACC] " + String(accOn ? "ON" : "OFF"));
        prevAccOn = accOn;
    }

    // Hand brake state change
    if (handBrakeEngaged != prevHandBrake) {
        Logger::info("[BRAKE] Hand brake " + String(handBrakeEngaged ? "engaged" : "released"));
        prevHandBrake = handBrakeEngaged;
    }

    // Door state change
    if (driverDoorOpen != prevDoorOpen) {
        Logger::info("[DOOR] Driver door " + String(driverDoorOpen ? "opened" : "closed"));
        prevDoorOpen = driverDoorOpen;
    }
}

bool VehicleStatusManager::verifyEngineStart() {
    delay(kStartVerifyDelayMs);

    // Force a fresh voltage reading before checking
    // The sensor task updates batteryVoltage every 80ms, so after 3s we have fresh data
    if (!batteryVoltage.hasReading()) {
        Logger::error("[ENGINE] Start failed: no battery reading");
        return false;
    }

    float voltage = batteryVoltage.getVoltage();
    engineStatus.batteryVoltage = voltage;

    // Direct voltage check (bypass hysteresis for start confirmation)
    bool started = (voltage > kEngineRunningThreshold);
    engineStatus.running = started;

    if (started) {
        Logger::info("[ENGINE] Start success");
    } else {
        Logger::info("[ENGINE] Start failed");
    }

    return started;
}

const char* VehicleStatusManager::getBatteryHealthStr() const {
    return batteryHealthToStr(classifyBatteryHealth(engineStatus.batteryVoltage));
}

String VehicleStatusManager::toJson() const {
    String json = "{";

    json += "\"engineRunning\":" + String(engineStatus.running ? "true" : "false") + ",";
    json += "\"batteryVoltage\":" + String(engineStatus.batteryVoltage, 1) + ",";
    json += "\"batteryHealth\":\"" + String(getBatteryHealthStr()) + "\",";
    json += "\"acc\":" + String(accOn ? "true" : "false") + ",";
    json += "\"handBrake\":" + String(handBrakeEngaged ? "true" : "false") + ",";
    json += "\"driverDoorOpen\":" + String(driverDoorOpen ? "true" : "false");

    json += "}";
    return json;
}
