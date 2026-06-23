#include "TaskManager.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "AuthManager.h"
#include "BatteryVoltage.h"
#include "Config.h"
#include "Logger.h"
#include "Metrics.h"
#include "NFCManager.h"
#include "RelayManager.h"
#include "RFManager.h"
#include "StatusLight.h"
#include "DisplayManager.h"
#include "StateMachine.h"
#include "WebManager.h"
#if ENABLE_BLE
#include "BLEManager.h"
#endif
#if ENABLE_CELLULAR
#include "CellularManager.h"
#include <esp_wifi.h>
#endif

namespace {

QueueHandle_t g_vehicleQueue = nullptr;

enum TaskId : uint8_t {
    TASK_VEHICLE = 0,
    TASK_NFC,
    TASK_WEB,
#if ENABLE_CELLULAR
    TASK_MQTT,
#endif
#if ENABLE_BLE
    TASK_BLE,
#endif
    TASK_SENSOR,
    TASK_WATCHDOG,
    TASK_COUNT
};

volatile uint32_t g_heartbeat[TASK_COUNT] = {0};
uint8_t g_watchdogStage[TASK_COUNT] = {0};

constexpr uint32_t kWatchdogWarnMs = 15000;
constexpr uint32_t kWatchdogSoftRecoverMs = 30000;
constexpr uint32_t kWatchdogHardRecoverMs = 60000;

inline void touch(TaskId id) {
    g_heartbeat[id] = millis();
}

const char* taskName(TaskId id) {
    switch (id) {
        case TASK_VEHICLE: return "VehicleTask";
        case TASK_NFC:     return "NfcTask";
        case TASK_WEB:     return "WebTask";
#if ENABLE_CELLULAR
        case TASK_MQTT:    return "MqttTask";
#endif
#if ENABLE_BLE
        case TASK_BLE:     return "BleTask";
#endif
        case TASK_SENSOR:  return "SensorTask";
        case TASK_WATCHDOG:return "WatchdogTask";
        default:           return "UnknownTask";
    }
}

uint32_t watchdogWarnMs(TaskId id) {
    if (id == TASK_WEB) {
        return 45000;
    }
    return kWatchdogWarnMs;
}

uint32_t watchdogSoftRecoverMs(TaskId id) {
    if (id == TASK_WEB) {
        return 90000;
    }
    return kWatchdogSoftRecoverMs;
}

uint32_t watchdogHardRecoverMs(TaskId id) {
    if (id == TASK_WEB) {
        return 180000;
    }
    return kWatchdogHardRecoverMs;
}

void softRecoverTask(TaskId id) {
    switch (id) {
#if ENABLE_CELLULAR
        case TASK_MQTT:
            Logger::warn("[Watchdog] soft recover MqttTask: reinit cellular manager");
            cellularManager.init();
            break;
#endif
#if ENABLE_BLE
        case TASK_BLE:
            Logger::warn("[Watchdog] soft recover BleTask: reinit BLE scan manager");
            bleManager.init();
            break;
#endif
        case TASK_WEB:
            Logger::warn("[Watchdog] soft recover WebTask: reinit web manager");
            //webManager.init();
            break;
        case TASK_NFC:
            Logger::warn("[Watchdog] soft recover NfcTask: reinit NFC manager");
            nfcManager.init();
            break;
        default:
            Logger::warn(String("[Watchdog] soft recover not supported for ") + taskName(id));
            break;
    }
}

void handleVehicleCommand(const VehicleCommand& cmd) {
    switch (cmd.type) {
        case VehicleCommandType::VEHICLE_CMD_START_FROM_WEB:
        case VehicleCommandType::VEHICLE_CMD_START_FROM_MQTT:
            if (batteryVoltage.isLowForRemoteStart()) {
                Logger::error("[VehicleTask] Start denied: low battery");
                Metrics::onRemoteStartResult(false, "low_battery");
#if ENABLE_CELLULAR
                if (cmd.type == VehicleCommandType::VEHICLE_CMD_START_FROM_MQTT) {
                    cellularManager.publishStatus("ERROR_LOW_BATTERY");
                }
#endif
                return;
            }
            if (relayManager.startEngine()) {
                Metrics::onRemoteStartResult(true, "ok");
                stateMachine.requireSecondaryAuth();
#if ENABLE_CELLULAR
                if (cmd.type == VehicleCommandType::VEHICLE_CMD_START_FROM_MQTT) {
                    cellularManager.publishStatus("ENGINE_STARTED_OK");
                }
#endif
            } else {
                Metrics::onRemoteStartResult(false, "relay_reject");
            }
            break;
        case VehicleCommandType::VEHICLE_CMD_LOCK_TOGGLE_FROM_NFC:
            StatusLight::setNFCSuccess();
            stateMachine.onAuthorizedNfc();
            break;
        case VehicleCommandType::AUTH_NFC_FAIL:
            StatusLight::setNFCError();
            stateMachine.onUnauthorizedNfc();
            break;
        default:
            break;
    }
}

void vehicleTask(void*) {
    touch(TASK_VEHICLE);
    VehicleCommand cmd{};
    for (;;) {
        while (xQueueReceive(g_vehicleQueue, &cmd, 0) == pdTRUE) {
            handleVehicleCommand(cmd);
        }
        stateMachine.update();
        touch(TASK_VEHICLE);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void nfcTask(void*) {
    touch(TASK_NFC);
    uint8_t lastUid[7] = {0};
    unsigned long lastCardMs = 0;
    unsigned long lastHeartbeat = 0;
    unsigned long lastScanMs = 0;
    for (;;) {
        touch(TASK_NFC);


        static bool lastNfcState = true;
        if (!authMethodNFC) {
            if (lastNfcState) {
                Logger::info("[NFC Task] NFC disabled, pausing scan");
                lastNfcState = false;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (!lastNfcState) {
            Logger::info("[NFC Task] NFC enabled, resuming scan");
            lastNfcState = true;
        }

        // NFC scan burst: reduce WiFi TX power during poll
        if (millis() - lastScanMs >= NFC_SCAN_INTERVAL_MS) {
            lastScanMs = millis();
            g_nfcScanning = true;
            RFManager::beginNFC();
            if (!StatusLight::isTransient()) StatusLight::setWaitingNFC();
            nfcManager.reconfig();
            delay(NFC_SCAN_SETTLE_MS);

            uint8_t uid[7] = {0};
            uint8_t uidLength = 0;
            bool detected = false;
            const unsigned long scanStart = millis();
            while (millis() - scanStart < NFC_SCAN_WINDOW_MS) {
                if (nfcManager.pollCard(uid, uidLength, NFC_SCAN_POLL_TIMEOUT_MS)) {
                    detected = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(2));
            }

            RFManager::endNFC();
            g_nfcScanning = false;

            if (detected) {
                unsigned long now = millis();
                if (uidLength > 0 && memcmp(uid, lastUid, uidLength) == 0 && (now - lastCardMs) < NFC_SAME_UID_COOLDOWN_MS) {
                    continue;
                }
                memcpy(lastUid, uid, uidLength);
                lastCardMs = now;

                String uidStr = "";
                for (uint8_t i = 0; i < uidLength; i++) {
                    if (uid[i] < 0x10) uidStr += "0";
                    uidStr += String(uid[i], HEX) + " ";
                }
                Logger::debug("[NFC] card UID: " + uidStr);

                StatusLight::setAuthenticating();
                if (AuthManager::verify(uid, uidLength)) {
                    TaskManager::sendVehicleCommand(VehicleCommandType::VEHICLE_CMD_LOCK_TOGGLE_FROM_NFC, 0);
                } else {
                    TaskManager::sendVehicleCommand(VehicleCommandType::AUTH_NFC_FAIL, 0);
                }
            }
        }
        touch(TASK_NFC);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void webTask(void*) {
    touch(TASK_WEB);
    for (;;) {
        webManager.handle();
        touch(TASK_WEB);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

#if ENABLE_CELLULAR
void mqttTask(void*) {
    touch(TASK_MQTT);
    cellularManager.init();
    for (;;) {
        cellularManager.update();
        touch(TASK_MQTT);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
#endif

#if ENABLE_BLE
void bleTask(void*) {
    touch(TASK_BLE);
    for (;;) {
        if (g_nfcScanning) {
            bleManager.stopActiveScan();  // NFC active: pause BLE scan
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        bleManager.update();
        if (bleManager.isScanningBLE()) { StatusLight::setBLEScanning(); }
        if (bleManager.isAuthorizedDeviceConnected()) { StatusLight::setBLEConnected(); }
        touch(TASK_BLE);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
#endif

void sensorTask(void*) {
    touch(TASK_SENSOR);
    for (;;) {
        batteryVoltage.update();
        relayManager.update();
        StatusLight::update();
        DisplayManager::update();
        touch(TASK_SENSOR);
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

void watchdogTask(void*) {
    touch(TASK_WATCHDOG);
    for (;;) {
        const uint32_t now = millis();
        for (uint8_t i = 0; i < TASK_COUNT - 1; i++) {
            TaskId id = static_cast<TaskId>(i);
            const uint32_t hb = g_heartbeat[i];
            if (hb == 0) continue;
            if (hb > now) continue;

            const uint32_t warnMs = watchdogWarnMs(id);
            const uint32_t softRecoverMs = watchdogSoftRecoverMs(id);
            const uint32_t hardRecoverMs = watchdogHardRecoverMs(id);
            const uint32_t elapsed = now - hb;
            if (elapsed <= warnMs) {
                if (g_watchdogStage[i] != 0) {
                    Logger::info(String("[Watchdog] ") + taskName(id) + " heartbeat recovered");
                    g_watchdogStage[i] = 0;
                }
                continue;
            }

            if (g_watchdogStage[i] < 1 && elapsed > warnMs) {
                Logger::error(String("[Watchdog] timeout warn: ") + taskName(id) + " elapsed_ms=" + String(elapsed));
                g_watchdogStage[i] = 1;
            }

            if (g_watchdogStage[i] < 2 && elapsed > softRecoverMs) {
                softRecoverTask(id);
                g_watchdogStage[i] = 2;
            }

            if (elapsed > hardRecoverMs) {
                Logger::error(String("[Watchdog] hard recover: reboot device due to ") + taskName(id) + " timeout");
                StatusLight::setError();
                delay(100);
                ESP.restart();
            }
        }
        Metrics::tick();
        touch(TASK_WATCHDOG);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

}  // namespace

void TaskManager::begin() {
    if (g_vehicleQueue != nullptr) return;

    g_vehicleQueue = xQueueCreate(12, sizeof(VehicleCommand));
    if (g_vehicleQueue == nullptr) {
        Logger::error("[TaskManager] Failed to create vehicle queue");
        return;
    }

    xTaskCreatePinnedToCore(vehicleTask, "VehicleTask", 6144, nullptr, 4, nullptr, 1);
    xTaskCreatePinnedToCore(nfcTask, "NfcTask", 4096, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(webTask, "WebTask", 6144, nullptr, 2, nullptr, 0);
#if ENABLE_CELLULAR
    xTaskCreatePinnedToCore(mqttTask, "MqttTask", 7168, nullptr, 2, nullptr, 0);
#endif
#if ENABLE_BLE
    xTaskCreatePinnedToCore(bleTask, "BleTask", 4096, nullptr, 1, nullptr, 0);
#endif
    xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, nullptr, 3, nullptr, 1);
    xTaskCreatePinnedToCore(watchdogTask, "WatchdogTask", 4096, nullptr, 1, nullptr, 0);

    Metrics::begin();
    Logger::info("[TaskManager] FreeRTOS tasks started");
}

bool TaskManager::sendVehicleCommand(VehicleCommandType type, uint32_t timeoutMs) {
    if (g_vehicleQueue == nullptr) return false;
    VehicleCommand cmd{type};
    TickType_t ticks = timeoutMs == 0 ? 0 : pdMS_TO_TICKS(timeoutMs);
    const bool queued = xQueueSend(g_vehicleQueue, &cmd, ticks) == pdTRUE;
    if (type == VehicleCommandType::VEHICLE_CMD_START_FROM_WEB ||
        type == VehicleCommandType::VEHICLE_CMD_START_FROM_MQTT) {
        if (queued) Metrics::onRemoteStartQueued();
        else Metrics::onRemoteStartResult(false, "queue_busy");
    }
    return queued;
}
