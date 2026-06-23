#include "StateMachine.h"

#include "Config.h"
#if ENABLE_BLE
#include "BLEManager.h"
#endif

#include "Logger.h"
#include "WebManager.h"

extern WebManager webManager;
#if ENABLE_BLE
extern BLEManager bleManager;
#endif

StateMachine::StateMachine(RelayManager* r) : relay(r), state(LOCKED) {}

void StateMachine::scheduleAlarmPulse(unsigned long durationMs) {
    relay->triggerAlarm();
    alarmPulseActive = true;
    alarmPulseUntilMs = millis() + durationMs;
}

void StateMachine::requireSecondaryAuth() {
    if (secAuthEnabled) {
        const bool nfcEnabled = authMethodNFC;
        const bool bleEnabled = authMethodBLE;
        if (!nfcEnabled && !bleEnabled) {
            pendingSecondaryAuth = false;
            Logger::warn("[AntiTheft] secondary auth enabled but no method active, bypassed");
            return;
        }

        pendingSecondaryAuth = true;
        String methods;
        if (bleEnabled) methods += "BLE";
        if (nfcEnabled) {
            if (methods.length() > 0) methods += "/";
            methods += "NFC";
        }
        Logger::info("[AntiTheft] secondary auth armed (" + methods + ", pass any one)");
    } else {
        Logger::info("[AntiTheft] secondary auth disabled by config");
    }
}

void StateMachine::clearPendingSecondaryAuth() {
    pendingSecondaryAuth = false;
}

void StateMachine::onAuthorizedNfc() {
    relay->stopAlarm();

    if (pendingSecondaryAuth) {
        if (!authMethodNFC) {
            Logger::warn("[AntiTheft] NFC detected but NFC secondary auth is disabled");
            return;
        }
        pendingSecondaryAuth = false;
        Logger::info("[AntiTheft] NFC secondary auth passed");
        scheduleAlarmPulse(100);
        return;
    }

    if (state == LOCKED) {
        state = UNLOCKED;
        relay->on();
        webManager.unlock();
        Logger::info("[StateMachine] vehicle unlocked");
    } else {
        state = LOCKED;
        relay->off();
        webManager.lock();
        Logger::info("[StateMachine] vehicle locked");
    }
}

void StateMachine::onUnauthorizedNfc() {
    Logger::warn("[StateMachine] NFC auth failed");
}

void StateMachine::update() {
    if (alarmPulseActive && static_cast<long>(millis() - alarmPulseUntilMs) >= 0) {
        relay->stopAlarm();
        alarmPulseActive = false;
    }

#if ENABLE_BLE
    if (pendingSecondaryAuth && authMethodBLE) {
        if (bleManager.isAuthorizedDeviceConnected()) {
            pendingSecondaryAuth = false;
            Logger::info("[AntiTheft] BLE secondary auth passed");
            bleManager.onBleAuthSuccess();
            relay->stopAlarm();
            scheduleAlarmPulse(100);
        }
    }
#endif

    if (relay->isEngineRunning() && pendingSecondaryAuth) {
        const bool isInNeutral = (digitalRead(PIN_NEUTRAL) == LOW);
        if (!isInNeutral) {
            Logger::error("[AntiTheft] unauthorized gear switch, stopping engine");
            relay->stopEngine();
            relay->triggerAlarm();
            pendingSecondaryAuth = false;
        }
    }
}
