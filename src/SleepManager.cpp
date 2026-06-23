#include "SleepManager.h"
#include "Config.h"
#include "Logger.h"
#include "RelayManager.h"
#include "StateMachine.h"
#if ENABLE_BLE
#include "BLEManager.h"
#endif
#include <esp_sleep.h>
#include <esp_wifi.h>

SleepManager sleepManager;

static unsigned long g_lastActivityMs = 0;
static unsigned long g_sleepEntryMs = 0;
static unsigned long g_nextWakeMs = 0;

void SleepManager::init() {
    g_lastActivityMs = millis();
    Logger::info("[Sleep] Light sleep mode: idle " + String(kIdleTimeoutMs / 1000) +
                 "s, wake every " + String(kSleepDurationMs / 1000) + "s");
}

static bool systemBusy() {
    extern RelayManager relayManager;
    extern StateMachine stateMachine;
#if ENABLE_BLE
    extern BLEManager bleManager;
#endif
    // Engine running = stay awake
    if (relayManager.isEngineRunning()) return true;
    // BLE authorized device nearby = stay awake
#if ENABLE_BLE
    if (bleManager.isAuthorizedDeviceConnected()) return true;
    if (bleManager.isPairing()) return true;
#endif
    // NFC config unlocked = stay awake
    if (!webAccessLocked) return true;
    return false;
}

void SleepManager::update() {
    if (systemBusy()) {
        g_lastActivityMs = millis();
        return;
    }

    unsigned long now = millis();
    if (now - g_lastActivityMs < kIdleTimeoutMs) return;

    // Enter light sleep
    g_sleepEntryMs = now;
    g_nextWakeMs = now + kSleepDurationMs;

    Logger::info("[Sleep] Idle " + String(kIdleTimeoutMs / 1000) +
                 "s, light sleep " + String(kSleepDurationMs / 1000) + "s...");

    // Put WiFi in modem-sleep before CPU sleep (AP buffers our packets)
    WiFi.setSleep(WIFI_PS_MAX_MODEM);

    esp_sleep_enable_timer_wakeup(kSleepDurationMs * 1000ULL);
    esp_light_sleep_start();

    // Restore WiFi to low-latency mode on wake
    WiFi.setSleep(WIFI_PS_MIN_MODEM);

    // Woke up
    unsigned long slept = millis() - g_sleepEntryMs;
    Logger::info("[Sleep] Woke after " + String(slept / 1000) + "s");
    g_lastActivityMs = millis();
}

bool SleepManager::isAwake() {
    if (systemBusy()) return true;
    return millis() - g_sleepEntryMs < 100;
}

unsigned long SleepManager::getSleepRemainingSec() {
    if (systemBusy()) return 0;
    if (g_nextWakeMs <= millis()) return 0;
    return (g_nextWakeMs - millis()) / 1000;
}
