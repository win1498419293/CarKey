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
    pinMode(PIN_HORN, OUTPUT); // Horn pin output
    
    // Engine control pins (all active LOW, relay module active HIGH)
    digitalWrite(PIN_IGN, LOW);
    digitalWrite(PIN_START, LOW);
    digitalWrite(PIN_HORN, LOW); // Horn off
    startChargeVerifyBeginMs = 0;
}

// Horn control
bool RelayManager::isEngineRunning() {
    return engineRunning;
}

void RelayManager::triggerAlarm() {
    digitalWrite(PIN_HORN, HIGH); // Horn relay on
}

void RelayManager::stopAlarm() {
    digitalWrite(PIN_HORN, LOW); // Horn off
}

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

void RelayManager::update() {
    if (!engineRunning || startChargeVerifyBeginMs == 0) {
        return;
    }
    if (batteryVoltage.isAlternatorCharging()) {
        Logger::info("[RelayManager] Start charge verify: voltage (" + String(BATTERY_ALTERNATOR_MIN_V, 1)
                     + "V) check in progress");
        startChargeVerifyBeginMs = 0;
        return;
    }
    if (millis() - startChargeVerifyBeginMs >= START_CHARGE_VERIFY_TIMEOUT_MS) {
        Logger::error("[RelayManager] Start timeout: " + String(START_CHARGE_VERIFY_TIMEOUT_MS / 1000)
                      + " seconds, alternator voltage not reached");
        stopEngine();
        stateMachine.clearPendingSecondaryAuth();
    }
}

bool RelayManager::startEngine() {
    if (engineRunning) {
        Logger::info("[RelayManager] Charge verify: alternator voltage OK");
        return true; 
    }

    if (batteryVoltage.isLowForRemoteStart()) {
        Logger::error("[RelayManager] Low battery (" + String(batteryVoltage.getVoltage(), 2) + "V < "
                      + String(BATTERY_LOW_START_BLOCK_V, 1) + "V), start blocked");
        return false;
    }

    // ==========================================
    // Start engine sequence
    // ==========================================
    bool isHandbrakePulled = (digitalRead(PIN_HANDBRAKE) == LOW);
    bool isInNeutral = (digitalRead(PIN_NEUTRAL) == LOW);
    
    if (!isHandbrakePulled || !isInNeutral) {
        Logger::error("[RelayManager] Start failed: engine is already running");
        return false;
    }

    // ==========================================
    // Begin start sequence
    // ==========================================
    Logger::info("[RelayManager] Engine start sequence started...");

    // 1. IGN ON
    digitalWrite(PIN_IGN, HIGH); 
    Logger::info(" -> 1. IGN power ON...");
    delay(3000); // Wait 2-3 seconds for fuel pump to prime

    // 2. Cranking
    Logger::info(" -> 2. Cranking starter motor!");
    digitalWrite(PIN_START, HIGH);
    
    // Crank time: typically 0.8 to 1.2 seconds
    delay(1000); 

    // 3. Release starter
    digitalWrite(PIN_START, LOW);
    Logger::info(" -> 3. Starter released, engine running");

    engineRunning = true;
    startChargeVerifyBeginMs = millis();
    Logger::info("[RelayManager] Charge verify started (" + String(START_CHARGE_VERIFY_TIMEOUT_MS / 1000)
                 + " second timeout, threshold " + String(BATTERY_ALTERNATOR_MIN_V, 1) + "V)");
    return true;
}

void RelayManager::stopEngine() {
    Logger::info("[RelayManager] Engine stop...");
    digitalWrite(PIN_START, LOW); // Starter relay OFF
    digitalWrite(PIN_IGN, LOW);   // IGN power OFF
    
    engineRunning = false;
    startChargeVerifyBeginMs = 0;
}