#include "RelayManager.h"
#include "Config.h"
#include "BatteryVoltage.h"
#include "StateMachine.h"
#include "Logger.h"
#include <Arduino.h>

extern BatteryVoltage batteryVoltage;
extern StateMachine stateMachine;

void RelayManager::init() {
    pinMode(RELAY_PIN, OUTPUT);
    off();
    pinMode(PIN_IGN, OUTPUT);
    pinMode(PIN_START, OUTPUT);
    pinMode(PIN_HORN, OUTPUT);

    // Engine control pins (all active HIGH for relay trigger)
    digitalWrite(PIN_IGN, LOW);
    digitalWrite(PIN_START, LOW);
    digitalWrite(PIN_HORN, LOW);
    startChargeVerifyBeginMs = 0;
    engineRelayActive = false;
}

// --- Horn control ---

void RelayManager::triggerAlarm() {
    digitalWrite(PIN_HORN, HIGH);
}

void RelayManager::stopAlarm() {
    digitalWrite(PIN_HORN, LOW);
}

// --- Main relay (door lock/unlock) ---

void RelayManager::on() {
    digitalWrite(RELAY_PIN, LOW);
    state = true;
}

void RelayManager::off() {
    digitalWrite(RELAY_PIN, HIGH);
    state = false;
}

bool RelayManager::isOn() {
    return state;
}

// --- Charge verification watchdog ---

void RelayManager::update() {
    if (!engineRelayActive || startChargeVerifyBeginMs == 0) {
        return;
    }
    if (batteryVoltage.isAlternatorCharging()) {
        Logger::info("[RelayManager] Start charge verify: voltage (>= " + String(BATTERY_ALTERNATOR_MIN_V, 1)
                     + "V), alternator OK");
        startChargeVerifyBeginMs = 0;
        return;
    }
    if (millis() - startChargeVerifyBeginMs >= START_CHARGE_VERIFY_TIMEOUT_MS) {
        Logger::error("[RelayManager] Start timeout: " + String(START_CHARGE_VERIFY_TIMEOUT_MS / 1000)
                      + "s, alternator voltage not reached");
        stopEngine();
        stateMachine.clearPendingSecondaryAuth();
    }
}

// --- Engine relay sequence ---
// Executes relay pulses for IGN + starter. VehicleStatusManager handles
// voltage-based engine running detection after this sequence completes.

bool RelayManager::startEngine() {
    if (engineRelayActive) {
        Logger::info("[RelayManager] Engine relay already active");
        return true;
    }

    if (batteryVoltage.isLowForRemoteStart()) {
        Logger::error("[RelayManager] Low battery (" + String(batteryVoltage.getVoltage(), 2) + "V < "
                      + String(BATTERY_LOW_START_BLOCK_V, 1) + "V), start blocked");
        return false;
    }

    // Safety checks
    bool isHandbrakePulled = (digitalRead(PIN_HANDBRAKE) == LOW);
    bool isInNeutral = (digitalRead(PIN_NEUTRAL) == LOW);

    if (!isHandbrakePulled || !isInNeutral) {
        Logger::error("[RelayManager] Start safety check failed: handbrake="
                      + String(isHandbrakePulled ? "OK" : "FAIL")
                      + " neutral=" + String(isInNeutral ? "OK" : "FAIL"));
        return false;
    }

    // --- Start sequence ---
    Logger::info("[RelayManager] Engine start sequence started...");

    // 1. IGN ON
    digitalWrite(PIN_IGN, HIGH);
    Logger::info(" -> 1. IGN power ON...");
    delay(3000);

    // 2. Cranking
    Logger::info(" -> 2. Cranking starter motor!");
    digitalWrite(PIN_START, HIGH);
    delay(1000);

    // 3. Release starter
    digitalWrite(PIN_START, LOW);
    Logger::info(" -> 3. Starter released");

    engineRelayActive = true;
    startChargeVerifyBeginMs = millis();
    Logger::info("[RelayManager] Start sequence complete, charge verify started ("
                 + String(START_CHARGE_VERIFY_TIMEOUT_MS / 1000) + "s timeout, threshold "
                 + String(BATTERY_ALTERNATOR_MIN_V, 1) + "V)");
    return true;
}

void RelayManager::stopEngine() {
    Logger::info("[RelayManager] Engine stop...");
    digitalWrite(PIN_START, LOW);
    digitalWrite(PIN_IGN, LOW);

    engineRelayActive = false;
    startChargeVerifyBeginMs = 0;
}
