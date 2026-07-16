#include "StatusJsonBuilder.h"
#include "Logger.h"
#include "VehicleStatus.h"
#include "BatteryVoltage.h"
#include "Config.h"
#if ENABLE_CELLULAR
#include "CellularManager.h"
#include "NetworkManager.h"
#endif
#if ENABLE_BLE
#include "BLEManager.h"
#endif

// Local JSON escape helper (same logic as WebManager::jsonEscape)
static String escapeJson(const String& input) {
    String out;
    out.reserve(input.length() + 8);
    for (size_t i = 0; i < input.length(); ++i) {
        const char ch = input[i];
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += ch;     break;
        }
    }
    return out;
}

// ========== buildBaseStatusString ==========
String buildBaseStatusString() {
    // Rate-limited cellular signal refresh (max every 10s)
    // Rate-limited cellular signal refresh (max every 3s)
    static unsigned long lastSignalRefresh = 0;
    unsigned long nowMs = millis();
#if ENABLE_CELLULAR
    if (nowMs - lastSignalRefresh >= 3000) {
        cellularManager.getSignalQuality();
        lastSignalRefresh = nowMs;
    }
#endif

    String j;
    j += "\"engineRunning\":" + String(vehicleStatus.isEngineRunning() ? "true" : "false") + ",";
    float v = batteryVoltage.hasReading() ? batteryVoltage.getVoltage() : 0.0f;
    j += "\"batteryVoltage\":" + String(v, 1) + ",";
    j += "\"voltage\":" + String(v, 1) + ",";
    j += "\"batteryHealth\":\"" + String(vehicleStatus.getBatteryHealthStr()) + "\",";
    j += "\"low_battery\":" + String(batteryVoltage.isLowForRemoteStart() ? "true" : "false") + ",";

    // ACC — critical for Web dashboard
    j += "\"acc\":" + String(vehicleStatus.isAccOn() ? "true" : "false") + ",";

    // Handbrake, door, gear
    j += "\"handbrake\":" + String(vehicleStatus.isHandBrakeEngaged() ? "true" : "false") + ",";
    j += "\"handBrake\":" + String(vehicleStatus.isHandBrakeEngaged() ? "true" : "false") + ",";
    j += "\"driverDoorOpen\":" + String(vehicleStatus.isDriverDoorOpen() ? "true" : "false") + ",";
    j += "\"gear\":\"" + String(vehicleStatus.isAccOn() ? "D" : "N") + "\",";

    // Engine field (snake_case for /api/status compat)
    j += "\"engine_running\":" + String(vehicleStatus.isEngineRunning() ? "true" : "false") + ",";

    // --- Cellular / Network status ---
#if ENABLE_CELLULAR
    j += "\"cellular_online\":" + String(networkManager.isCellularOnline() ? "true" : "false") + ",";
    j += "\"cellular_ready\":" + String(cellularManager.isModemReady() ? "true" : "false") + ",";
    j += "\"mqtt\":" + String(networkManager.isOnline() ? "true" : "false") + ",";
    j += "\"csq\":" + String(cellularManager.getCurrentSignal().rssi) + ",";
    j += "\"rssi_dbm\":" + String(cellularManager.getCurrentSignal().rssiDbm) + ",";
    j += "\"cereg\":" + String(cellularManager.isNetworkRegistered() ? 1 : 0) + ",";
    j += "\"signal\":\"" + String(cellularManager.getCurrentSignal().level) + "\",";
#else
    j += "\"cellular_online\":false,\"cellular_ready\":false,\"mqtt\":false,";
    j += "\"csq\":0,\"rssi_dbm\":0,\"cereg\":0,\"signal\":\"none\",";
#endif

    // --- Config / settings fields ---
    j += "\"config_locked\":" + String(webAccessLocked ? "true" : "false") + ",";
    j += "\"locked\":" + String(webAccessLocked ? "true" : "false") + ",";
    j += "\"wifi_ssid\":\"" + escapeJson(wifi_ssid) + "\",";
    j += "\"bt_name\":\"" + escapeJson(bt_name) + "\",";
    j += "\"sec_auth\":" + String(secAuthEnabled ? "true" : "false") + ",";
    j += "\"ble_scan\":" + String(bleScanEnabled ? "true" : "false") + ",";
    j += "\"nfc_scan\":" + String(authMethodNFC ? "true" : "false") + ",";
    j += "\"build_stamp\":\"" + String(kBuildStamp) + "\",";
    j += "\"ip\":\"" + WiFi.localIP().toString() + "\",";

    // --- BLE fields ---
#if ENABLE_BLE
    j += "\"ble_authorized\":" + String(bleManager.isAuthorizedDeviceConnected() ? "true" : "false") + ",";
    j += "\"ble_auth_valid\":" + String(bleManager.isAuthorizedDeviceConnected() ? "true" : "false") + ",";
    j += "\"ble_scanning\":" + String(bleManager.isScanningBLE() ? "true" : "false") + ",";
    j += "\"ble_last_seen\":" + String(bleManager.getLastSeenSec()) + ",";
    j += "\"ble_ready\":" + String(bleManager.isReady() ? "true" : "false") + ",";
    j += "\"ble_cooldown_active\":" + String(bleManager.isScanCooldownActive() ? "true" : "false") + ",";
    j += "\"ble_cooldown_remaining_ms\":" + String(bleManager.getScanCooldownRemainingMs());
#else
    j += "\"ble_authorized\":false,\"ble_auth_valid\":false,\"ble_scanning\":false,";
    j += "\"ble_last_seen\":-1,\"ble_ready\":false,\"ble_cooldown_active\":false,\"ble_cooldown_remaining_ms\":0";
#endif
    // Note: NO trailing comma or brace — caller adds wrap

    return j;
}

// ========== buildStatusString (mode-aware) ==========
String buildStatusString(BuilderMode mode) {
    String base = buildBaseStatusString();

    switch (mode) {
        case BuilderMode::API_OR_WS:
            // Full status — base has everything
            return "{" + base + "}";

        case BuilderMode::CELLULAR_MQTT: {
            // Base already has csq/rssi/signal/cereg/ip from last poll.
            // Here we append the Cellular-specific extra fields.
            // NOTE: If caller wants real-time CEREG/IP, it should query
            // AT+CGPADDR / AT+CEREG? before calling and append overrides.
            String j = "{" + base;
            j += ",\"uptime\":" + String(millis() / 1000);
            // GNSS if available (from CellularManager cached value)
#if ENABLE_CELLULAR
            if (cellularManager.getLastGNSSInfo().valid) {
                j += ",\"lat\":" + String(cellularManager.getLastGNSSInfo().latitude, 6);
                j += ",\"lng\":" + String(cellularManager.getLastGNSSInfo().longitude, 6);
                j += ",\"speed\":" + String(cellularManager.getLastGNSSInfo().speed, 1);
                j += ",\"sat\":" + String(cellularManager.getLastGNSSInfo().satellites);
            }
#endif
            j += "}";
            return j;
        }

        case BuilderMode::WIFI_MQTT: {
            // WiFi MQTT publishes minimal status
            String j = "{" + base;
            j += ",\"uptime\":" + String(millis() / 1000);
            j += "}";
            return j;
        }

        default:
            return "{" + base + "}";
    }
}
