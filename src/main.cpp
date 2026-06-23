#include <Arduino.h>
#include <esp_wifi.h>
#include <soc/soc.h>

#include "BatteryVoltage.h"
#include "Config.h"
#if ENABLE_BLE
#include "BLEManager.h"
#endif
#include "Logger.h"
#include "NFCManager.h"
#include "RelayManager.h"
#include "StateMachine.h"
#include "TaskManager.h"
#include "WebManager.h"
#include "OTAManager.h"
#include "StatusLight.h"
#include "DisplayManager.h"
#include "SleepManager.h"

NFCManager nfcManager;
RelayManager relayManager;
StateMachine stateMachine(&relayManager);

void setup() {
    delay(3000);
    Serial.begin(115200); delay(100);

#ifdef CONFIG_IDF_TARGET_ESP32
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
#endif

    StatusLight::init();
    loadSystemConfig();
    otaManager.init();

    pinMode(PIN_HANDBRAKE, INPUT_PULLUP);
    pinMode(PIN_NEUTRAL, INPUT_PULLUP);

    batteryVoltage.init();
    relayManager.init();
#if ENABLE_BLE
    bleManager.init();
#endif
    DisplayManager::init();
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    webManager.init();
    nfcManager.init();

    TaskManager::begin();
    StatusLight::setIdle();
    sleepManager.init();
    Logger::info("DONE");
}

void loop() {
    // Light sleep manager: enters sleep when idle, wakes every 30s
    sleepManager.update();
    vTaskDelay(pdMS_TO_TICKS(500));
}