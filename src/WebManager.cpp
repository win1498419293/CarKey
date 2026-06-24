#include "WebManager.h"
#include "Config.h"
#include "RelayManager.h"
#include "StateMachine.h"
#include "BatteryVoltage.h"
#include "VehicleStatus.h"
#include "Logger.h"
#include "TaskManager.h"
#include "OTAManager.h"
#include "StatusLight.h"
#include "NFCManager.h"
#include "RFManager.h"
#include "EmbeddedIndexPage.h"
#if ENABLE_BLE
#include "BLEManager.h"
#endif

extern RelayManager relayManager;
extern StateMachine stateMachine;
extern BatteryVoltage batteryVoltage;
extern VehicleStatusManager vehicleStatus;
extern NFCManager nfcManager;
#if ENABLE_BLE
extern BLEManager bleManager;
#endif

WebServer server(80);

namespace {

void sendNoCacheHeaders();
bool g_spiffsReady = false;
bool g_wifiEventRegistered = false;
bool g_routesReady = false;
bool g_serverStarted = false;
bool g_hasLoggedIp = false;
unsigned long g_lastWifiConnectAttemptMs = 0;
unsigned long g_lastSpiffsMountAttemptMs = 0;
unsigned long g_lastWifiBeginMs = 0;
bool g_wifiRadioOff = false;
unsigned long g_wifiConnectedSinceMs = 0;
bool g_wasUnlocked = false;  // Track previous unlock state for auth change detection
constexpr unsigned long kWifiReconnectIntervalMs = 60000;
constexpr unsigned long kWifiMaxConnectedMs = 120000;  // 60s cycle limit, yield RF to NFC after
constexpr unsigned long kSpiffsRetryIntervalMs = 15000;

String jsonEscape(const String& input) {
    String out;
    out.reserve(input.length() + 8);
    for (size_t i = 0; i < input.length(); ++i) {
        const char ch = input[i];
        switch (ch) {
            case '\\':
                out += "\\\\";
                break;
            case '\"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += ch;
                break;
        }
    }
    return out;
}

String normalizeSpiffsPath(const String& rawPath) {
    if (rawPath.length() == 0) {
        return "/";
    }
    return rawPath.startsWith("/") ? rawPath : "/" + rawPath;
}

File openSpiffsFile(const String& rawPath) {
    const String normalized = normalizeSpiffsPath(rawPath);
    if (SPIFFS.exists(normalized)) {
        return SPIFFS.open(normalized, FILE_READ);
    }

    if (normalized.length() > 1) {
        const String alternate = normalized.substring(1);
        if (SPIFFS.exists(alternate)) {
            return SPIFFS.open(alternate, FILE_READ);
        }
    }

    File root = SPIFFS.open("/");
    if (root) {
        const String targetWithSlash = normalized;
        const String targetWithoutSlash = normalized.length() > 1 ? normalized.substring(1) : normalized;
        for (File file = root.openNextFile(); file; file = root.openNextFile()) {
            const String candidate = String(file.name());
            if (candidate.equalsIgnoreCase(targetWithSlash) || candidate.equalsIgnoreCase(targetWithoutSlash)) {
                file.close();
                return SPIFFS.open(candidate, FILE_READ);
            }
        }
    }

    return File();
}

void logSpiffsRootFiles() {
    File root = SPIFFS.open("/");
    if (!root) {
        Logger::warn("[WebManager] Unable to open SPIFFS root for listing");
        return;
    }

    File file = root.openNextFile();
    if (!file) {
        Logger::warn("[WebManager] SPIFFS root is empty");
    }

    while (file) {
        Logger::info("[WebManager] SPIFFS file: " + String(file.name()) + " (" + String(file.size()) + " bytes)");
        file = root.openNextFile();
    }
}

bool ensureSpiffsMounted(bool forceRetry = false) {
    const unsigned long now = millis();
    if (g_spiffsReady && SPIFFS.totalBytes() > 0) {
        return true;
    }
    if (!forceRetry && (now - g_lastSpiffsMountAttemptMs) < kSpiffsRetryIntervalMs) {
        return false;
    }

    g_lastSpiffsMountAttemptMs = now;
    g_spiffsReady = SPIFFS.begin(true);
    if (!g_spiffsReady) {
        Logger::error("[WebManager] SPIFFS mount failed; keeping fallback page and preserving flash contents");
        return false;
    }

    Logger::info("[WebManager] SPIFFS mounted, total=" + String(SPIFFS.totalBytes()) +
                 " used=" + String(SPIFFS.usedBytes()));
    logSpiffsRootFiles();
    return true;
}

void ensureWifiConnected() {
    if (wifi_ssid.length() == 0) return;
    if (WiFi.status() == WL_CONNECTED) return;

    static unsigned long lastAttemptMs = 0;
    unsigned long now = millis();
    if (now - lastAttemptMs < 10000) return;  // 10 second cooldown between reconnect attempts
    lastAttemptMs = now;

    Logger::info("[WebManager] Connecting WiFi to SSID: " + wifi_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
}

bool sendIfConfigLocked() {
    if (!webAccessLocked) {
        return false;
    }
    sendNoCacheHeaders();
    server.send(403, "text/plain",
                 "CONFIG_LOCKED: Swipe authorized NFC on vehicle to unlock WiFi/OTA/settings.");
    return true;
}

bool g_otaRejectedByConfigLock = false;

void sendNoCacheHeaders() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
}

}  // namespace

WebManager webManager;

// --- SPIFFS / gzip static file serving ---
// --- SPIFFS / gzip static file fallback serving ---
void WebManager::serveStaticFile(const char* path, const char* contentType) {
    sendNoCacheHeaders();
    const String requestedPath = normalizeSpiffsPath(String(path));

    if (requestedPath == "/index.html" || requestedPath == "/") {
        server.send_P(200, "text/html; charset=utf-8", kEmbeddedIndexPage);
        return;
    }

    if (ensureSpiffsMounted()) {
        File plainFile = openSpiffsFile(requestedPath);
        if (plainFile) {
            server.streamFile(plainFile, contentType);
            plainFile.close();
            return;
        }

        // Prefer plain HTML for now so stale or mismatched .gz artifacts do not shadow new UI builds.
    }

    Logger::error("[WebManager] SPIFFS file not found: " + requestedPath);
    server.send(404, "text/plain", "404 Not Found");
    return;

    // Check if client supports gzip encoding
    bool clientSupportsGzip = false;
    if (server.hasHeader("Accept-Encoding")) {
        String ae = server.header("Accept-Encoding");
        if (ae.indexOf("gzip") >= 0) {
            clientSupportsGzip = true;
        }
    }

    // Serve gzip compressed file if available
    if (clientSupportsGzip) {
        String gzPath = String(path) + ".gz";
        if (SPIFFS.exists(gzPath)) {
            File file = SPIFFS.open(gzPath, "r");
            if (file) {
                server.sendHeader("Content-Encoding", "gzip");
                server.streamFile(file, contentType);
                file.close();
                return;
            }
        }
    }

    // Serve plain file from SPIFFS
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        if (file) {
            server.streamFile(file, contentType);
            file.close();
            return;
        }
    }

    // SPIFFS file not found, serve embedded fallback
    Logger::error("[WebManager] SPIFFS file not found: " + String(path));
        // Fallback: serve embedded PROGMEM HTML for index page
    Logger::error("[WebManager] SPIFFS file not found: " + String(path));
    if (String(path) == "/index.html" || String(path) == "/") {
        sendNoCacheHeaders();
        server.send_P(200, "text/html; charset=utf-8", kEmbeddedIndexPage);
        return;
    }
    server.send(404, "text/plain", "404 Not Found");
}

// --- Build info for debugging ---
constexpr const char* kBuildStamp = __DATE__ " " __TIME__;

void WebManager::init() {
    ensureSpiffsMounted(true);

    if (!g_wifiEventRegistered) {
        WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
            switch (event) {
                case ARDUINO_EVENT_WIFI_STA_START:
                    Logger::info("[WiFi] STA started");
                    break;
                case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                    Logger::info("[WiFi] STA connected to AP");
                    break;
                case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                    Logger::info("[WiFi] Got IP: " + WiFi.localIP().toString());
                    g_hasLoggedIp = false;
                    break;
                case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                    g_hasLoggedIp = false;
                    Logger::warn("[WiFi] Disconnected, reason=" + String(info.wifi_sta_disconnected.reason) + " SSID=" + wifi_ssid);
                    break;
                default:
                    break;
            }
        });
        g_wifiEventRegistered = true;
    }

    // WiFi and NFC coordination: init RF and PN532 before WiFi connection





    if (!g_routesReady) {
        setupRoutes();
        g_routesReady = true;
    }

    if (!g_serverStarted) {
    WiFi.mode(WIFI_STA);  // Initialize TCP/IP stack, then WiFi
        server.begin();
        g_serverStarted = true;
        Logger::info("[WebManager] HTTP server started");
    }
    return;
    // Fall through: SPIFFS mount and WiFi connection
    if (!SPIFFS.begin(true)) {
        Logger::error("[WebManager] SPIFFS mount failed!");
    } else {
        Logger::info("[WebManager] SPIFFS mounted, total=" + String(SPIFFS.totalBytes()) +
                     " used=" + String(SPIFFS.usedBytes()));
    }

    Logger::info("[WebManager] Connecting WiFi to SSID: " + wifi_ssid);
    Logger::info("[WebManager] WiFi password: " + wifi_pass);
    setCpuFrequencyMhz(80);
    delay(50);
    WiFi.mode(WIFI_STA);
    WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_START:
                Logger::info("[WiFi] STA started");
                break;
            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                Logger::info("[WiFi] STA connected to AP");
                break;
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                Logger::info("[WiFi] Got IP: " + WiFi.localIP().toString());
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                Logger::warn("[WiFi] Disconnected, reason=" + String(info.wifi_sta_disconnected.reason) + " SSID=" + wifi_ssid);
                break;
            default: break;
        }
    });
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    setCpuFrequencyMhz(240);
    setupRoutes();
    server.begin();
}

void WebManager::setupRoutes() {
    // Root route - serve embedded page as SPIFFS fallback
    server.on("/", []() {
        sendNoCacheHeaders();
        server.send_P(200, "text/html; charset=utf-8", kEmbeddedIndexPage);
    });

    server.on("/api/start_engine", []() {
        if (!server.hasArg("pwd") || server.arg("pwd") != start_pwd) {
            sendNoCacheHeaders();
            Logger::warn("[Web] Engine start denied: no password");
            server.send(401, "text/plain", "Unauthorized: Wrong Password");
            return;
        }

        if (batteryVoltage.isLowForRemoteStart()) {
            sendNoCacheHeaders();
            Logger::error("[Web] Low battery(" + String(batteryVoltage.getVoltage(), 2) + "V)");
            server.send(503, "text/plain", "LOW_BATTERY");
            return;
        }

        Logger::info("[Web] Remote engine start requested");

        // Execute relay sequence directly (synchronous)
        if (!relayManager.startEngine()) {
            sendNoCacheHeaders();
            server.send(503, "text/plain", "START_SEQUENCE_FAILED");
            return;
        }

        // V1.1: Wait and verify engine actually started via voltage
        bool started = vehicleStatus.verifyEngineStart();

        if (started) {
            stateMachine.requireSecondaryAuth();
            sendNoCacheHeaders();
            server.send(200, "text/plain", "ENGINE_STARTED");
        } else {
            relayManager.stopEngine();
            sendNoCacheHeaders();
            server.send(503, "text/plain", "ENGINE_START_FAILED");
        }
    });

    server.on("/api/stop_engine", []() {
        if (!server.hasArg("pwd") || server.arg("pwd") != start_pwd) {
            sendNoCacheHeaders();
            Logger::warn("[Web] Engine stop denied");
            server.send(401, "text/plain", "Unauthorized: Wrong Password");
            return;
        }

        if (!vehicleStatus.isEngineRunning()) {
            sendNoCacheHeaders();
            server.send(409, "text/plain", "ENGINE_NOT_RUNNING");
            return;
        }

        relayManager.stopEngine();
        stateMachine.clearPendingSecondaryAuth();
        Logger::info("[Web] Engine stop requested");
        sendNoCacheHeaders();
        server.send(200, "text/plain", "ENGINE_STOPPED");
    });

    server.on("/api/lock", []() {
        Logger::info("[Web] Lock requested");
        sendNoCacheHeaders();
        server.send(200, "text/plain", "Locked");
    });

    server.on("/api/unlock", []() {
        Logger::info("[Web] Lock toggled");
        sendNoCacheHeaders();
        server.send(200, "text/plain", "Unlocked");
    });
    server.on("/api/window", []() {
        Logger::info("[Web] Window toggle");
        sendNoCacheHeaders();
        server.send(200, "text/plain", "Window Toggled");
    });


    server.on("/api/status", []() {
        bool isHandbrakePulled = (digitalRead(PIN_HANDBRAKE) == LOW);
        bool isInNeutral = (digitalRead(PIN_NEUTRAL) == LOW);

        float currentVoltage = batteryVoltage.hasReading() ? batteryVoltage.getVoltage() : 0.0f;
        bool lowBattery = batteryVoltage.isLowForRemoteStart();
        String gearState = isInNeutral ? "N" : "D";

        String json = "{";
        json += "\"voltage\":" + String(currentVoltage, 1) + ",";
        json += "\"low_battery\":" + String(lowBattery ? "true" : "false") + ",";
        json += "\"handbrake\":" + String(isHandbrakePulled ? "true" : "false") + ",";
        json += "\"gear\":\"" + gearState + "\",";
        json += "\"engine_running\":" + String(vehicleStatus.isEngineRunning() ? "true" : "false") + ",";
        json += "\"config_locked\":" + String(webAccessLocked ? "true" : "false") + ",";
        json += "\"locked\":" + String(webAccessLocked ? "true" : "false") + ",";
        json += "\"wifi_ssid\":\"" + jsonEscape(wifi_ssid) + "\",";
        json += "\"bt_name\":\"" + jsonEscape(bt_name) + "\",";
        json += "\"sec_auth\":" + String(secAuthEnabled ? "true" : "false") + ",";
        json += "\"ble_scan\":" + String(bleScanEnabled ? "true" : "false") + ",";
        json += "\"nfc_scan\":" + String(authMethodNFC ? "true" : "false") + ",";
        json += "\"build_stamp\":\"" + String(kBuildStamp) + "\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        #if ENABLE_BLE
        json += "\"ble_scanning\":" + String(bleManager.isScanningBLE() ? "true" : "false");
        json += ",\"ble_ready\":" + String(bleManager.isReady() ? "true" : "false");
        json += ",\"ble_auth_valid\":" + String(bleManager.isAuthorizedDeviceConnected() ? "true" : "false");
        json += ",\"ble_authorized\":" + String(bleManager.isAuthorizedDeviceConnected() ? "true" : "false");
        json += ",\"ble_last_seen\":" + String(bleManager.getLastSeenSec());
        json += ",\"ble_cooldown_active\":" + String(bleManager.isScanCooldownActive() ? "true" : "false");
        json += ",\"ble_cooldown_remaining_ms\":" + String(bleManager.getScanCooldownRemainingMs());
#else
        json += "\"ble_scanning\":\"false\"";
        json += ",\"ble_ready\":false";
        json += ",\"ble_auth_valid\":false";
        json += ",\"ble_authorized\":false";
        json += ",\"ble_last_seen\":-1";
        json += ",\"ble_cooldown_active\":false";
        json += ",\"ble_cooldown_remaining_ms\":0";
#endif
        json += "}";
        sendNoCacheHeaders();
        server.send(200, "application/json", json);
    });

    server.on("/api/get_config", []() {
        if (sendIfConfigLocked()) {
            return;
        }
        String json = "{";
        json += "\"ssid\":\"" + jsonEscape(wifi_ssid) + "\",";
        json += "\"wifi_ssid\":\"" + jsonEscape(wifi_ssid) + "\",";
        json += "\"bt_name\":\"" + jsonEscape(bt_name) + "\",";
        json += "\"sec_auth\":" + String(secAuthEnabled ? "true" : "false") + ",";
        json += "\"ble_scan\":" + String(bleScanEnabled ? "true" : "false") + ",";
        json += "\"nfc_scan\":" + String(authMethodNFC ? "true" : "false") + ",";
        json += "\"sec_auth\":" + String(secAuthEnabled ? "true" : "false");
        json += ",\"ble_scan\":" + String(bleScanEnabled ? "true" : "false");
        json += "}";
        sendNoCacheHeaders();
        server.send(200, "application/json", json);
    });

    server.on("/api/update_config", HTTP_POST, []() {
        if (sendIfConfigLocked()) {
            return;
        }
        if (server.hasArg("new_pwd")) {
            String newPwd = server.arg("new_pwd");
            if (newPwd.length() > 0) {
                start_pwd = newPwd;
            }
        }
        if (server.hasArg("new_ssid")) {
            String newSsid = server.arg("new_ssid");
            if (newSsid.length() > 0) {
                wifi_ssid = newSsid;
            }
        }
        if (server.hasArg("new_pass")) {
            String newPass = server.arg("new_pass");
            if (newPass.length() > 0) {
                wifi_pass = newPass;
            }
        }
        if (server.hasArg("new_bt")) {
            String newBt = server.arg("new_bt");
            if (newBt.length() > 0) {
                bt_name = newBt;
            }
        }
        if (server.hasArg("sec_auth")) secAuthEnabled = (server.arg("sec_auth") == "true");
        if (server.hasArg("ble_scan")) {
            bleScanEnabled = (server.arg("ble_scan") == "true");
            authMethodBLE = bleScanEnabled;
            if (!bleScanEnabled) {
                bleManager.stopActiveScan();
            } else {
                bleManager.init();
            }
        }
        if (server.hasArg("nfc_scan")) {
            authMethodNFC = (server.arg("nfc_scan") == "true");
            Logger::info(String("[Config] NFC scan ") + (authMethodNFC ? "ENABLED" : "DISABLED"));
        }

        bool saved = saveSystemConfig();
        if (!saved) {
            Logger::error("[System] Config save FAILED!");
            sendNoCacheHeaders();
            server.send(500, "text/plain", "Save Failed: NVS error");
            return;
        }
        Logger::info("[System] Config saved to NVS");

    // WiFi SSID / password update - restart if changed
        ensureWifiConnected();

        sendNoCacheHeaders();
        server.send(200, "text/plain", "Settings Saved");
    });

    server.on("/api/logs", []() {
        String logs = takeWebLogBuffer();
        sendNoCacheHeaders();
        server.send(200, "text/plain", logs);
    });


    server.on("/api/wifi_scan", []() {
        sendNoCacheHeaders();
        String json = webManager.scanWiFi();
        server.send(200, "application/json", json);
    });

    server.on("/api/ota", HTTP_POST, []() {
        if (g_otaRejectedByConfigLock) {
            g_otaRejectedByConfigLock = false;
            sendNoCacheHeaders();
            server.send(403, "text/plain", "CONFIG_LOCKED: Swipe authorized NFC on vehicle first.");
            return;
        }
        bool success = (otaManager.getState() == OTAState::OTA_COMPLETE_PENDING);
        sendNoCacheHeaders();
        server.send(200, "text/plain", success ? "OTA Success, Rebooting..." : "OTA Failed");
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            g_otaRejectedByConfigLock = webAccessLocked;
            if (g_otaRejectedByConfigLock) {
                Logger::warn("[OTA] Rejected: config locked, swipe NFC first");
                return;
            }
            StatusLight::setOTA();
            Logger::info("[OTA] Receiving: " + upload.filename);
            if (!otaManager.begin()) {
                Logger::error("[OTA] otaManager.begin() failed");
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (g_otaRejectedByConfigLock) return;
            otaManager.write(upload.buf, upload.currentSize);
        } else if (upload.status == UPLOAD_FILE_END) {
            if (g_otaRejectedByConfigLock) return;
            if (otaManager.end()) {
                Logger::info("[OTA] Write success, " + String(upload.totalSize) + " bytes");
            } else {
                StatusLight::setIdle();
                Logger::error("[OTA] Write failed: " + otaManager.getLastError());
            }
        }
    });

    server.on("/api/ota/status", []() {
        String json = "{";
        json += "\"state\":" + String(static_cast<int>(otaManager.getState())) + ",";
        json += "\"updating\":" + String(otaManager.isUpdating() ? "true" : "false") + ",";
        json += "\"progress\":" + String(otaManager.getProgressPercent()) + ",";
        json += "\"version\":\"" + jsonEscape(otaManager.getRunningVersion()) + "\",";
        json += "\"partition\":\"" + jsonEscape(otaManager.getRunningPartitionLabel()) + "\",";
        json += "\"stored_fw\":" + String(otaManager.hasStoredFirmware() ? "true" : "false") + ",";
        json += "\"error\":\"" + jsonEscape(otaManager.getLastError()) + "\"";
        json += "}";
        sendNoCacheHeaders();
        server.send(200, "application/json", json);
    });

    server.on("/api/ota/download", HTTP_POST, []() {
        if (sendIfConfigLocked()) return;
        String url = server.arg("url");
        if (url.length() == 0) {
            sendNoCacheHeaders();
            server.send(400, "text/plain", "Missing url parameter");
            return;
        }
        Logger::info("[OTA] URL download requested: " + url);
        StatusLight::setOTA();
        sendNoCacheHeaders();
        server.send(200, "text/plain", "Download started, device will reboot on success");
        delay(500);
        if (otaManager.downloadAndApply(url)) {
            Logger::info("[OTA] URL download success, rebooting...");
            delay(1000);
            ESP.restart();
        } else {
            StatusLight::setIdle();
            Logger::error("[OTA] URL download failed: " + otaManager.getLastError());
        }
    });


    // OTA rollback
    server.on("/api/ota/rollback", []() {
        if (sendIfConfigLocked()) return;
        sendNoCacheHeaders();
        if (!otaManager.hasStoredFirmware()) {
            server.send(400, "application/json", "{\"success\":false,\"message\":\"No stored firmware to rollback to\"}");
            return;
        }
        if (otaManager.rollback()) {
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Rolling back, rebooting...\"}");
            delay(1000);
            ESP.restart();
        } else {
            server.send(500, "application/json", "{\"success\":false,\"message\":\"" + otaManager.getLastError() + "\"}");
        }
    });
    // After WiFi connected, yield airtime to NFC
    
    // BLE pairing API endpoints
    server.on("/api/ble/pairing/start", []() {
        if (sendIfConfigLocked()) return;
        sendNoCacheHeaders();
        bool ok = bleManager.startPairing();
        server.send(200, "application/json", "{\"success\":" + String(ok ? "true" : "false") + ",\"message\":\"" + (ok ? "Pairing started, search for " + bt_name : "Pairing failed") + "\"}");
    });

    server.on("/api/ble/pairing/stop", []() {
        if (sendIfConfigLocked()) return;
        sendNoCacheHeaders();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Pairing stopped\"}");
    });

    server.on("/api/ble/pairing/status", []() {
        sendNoCacheHeaders();
        String json = "{";
        json += "\"wifi_ssid\":\"" + jsonEscape(wifi_ssid) + "\",";
        json += "\"bt_name\":\"" + jsonEscape(bt_name) + "\",";
        json += "\"sec_auth\":" + String(secAuthEnabled ? "true" : "false") + ",";
        json += "\"ble_scan\":" + String(bleScanEnabled ? "true" : "false") + ",";
        json += "\"nfc_scan\":" + String(authMethodNFC ? "true" : "false") + ",";
        json += "\"pairing\":" + String(bleManager.isPairing() ? "true" : "false") + ",";
        json += "\"paired\":" + String(bleManager.isPaired() ? "true" : "false") + ",";
        json += "\"name\":\"" + jsonEscape(bleManager.getPairedDeviceName()) + "\",";
        json += String("\"mac\":\"") + bleManager.getPairedDeviceMac() + "\"";
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/api/ble/pairing/clear", []() {
        if (sendIfConfigLocked()) return;
        sendNoCacheHeaders();
        bleManager.clearPairing();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Pairing cleared\"}");
    });

server.on("/api/system/info", []() {
        sendNoCacheHeaders();
        String json = "{";
        // RAM
        json += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"heap_total\":" + String(ESP.getHeapSize()) + ",";
        json += "\"heap_min_free\":" + String(ESP.getMinFreeHeap()) + ",";
        json += "\"heap_max_alloc\":" + String(ESP.getMaxAllocHeap()) + ",";
        // PSRAM
#ifdef BOARD_HAS_PSRAM
        json += "\"psram_size\":" + String(ESP.getPsramSize()) + ",";
        json += "\"psram_free\":" + String(ESP.getFreePsram()) + ",";
        json += "\"psram_min_free\":" + String(ESP.getMinFreePsram()) + ",";
        json += "\"psram_max_alloc\":" + String(ESP.getMaxAllocPsram()) + ",";
#else
        json += "\"psram_size\":0,";
        json += "\"psram_free\":0,";
        json += "\"psram_min_free\":0,";
        json += "\"psram_max_alloc\":0,";
#endif
        // Flash
        json += "\"flash_size\":" + String(ESP.getFlashChipSize()) + ",";
        json += "\"flash_speed\":" + String(ESP.getFlashChipSpeed()) + ",";
        json += "\"sketch_size\":" + String(ESP.getSketchSize()) + ",";
        json += "\"sketch_free\":" + String(ESP.getFreeSketchSpace()) + ",";
        json += "\"spiffs_total\":" + String(SPIFFS.totalBytes()) + ",";
        json += "\"spiffs_used\":" + String(SPIFFS.usedBytes());
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/api/reboot", []() {
        sendNoCacheHeaders();
        server.send(200, "text/plain", "Rebooting...");
        delay(500);
        ESP.restart();
    });

}

void WebManager::handle() {
    ensureSpiffsMounted(false);

    if (WiFi.status() == WL_CONNECTED) {
        if (!g_hasLoggedIp) {
            Logger::info("[WebManager] WiFi connected, IP: " + WiFi.localIP().toString());
            g_hasLoggedIp = true;
        }

        // WiFi connected: block config access when WiFi is active
        if (webAccessLocked) {
            // RFManager and NFC query coordination when WiFi is connected
            server.handleClient();
            return;
        }

        server.handleClient();
        return;
    }

    // Initial WiFi connection phase: coordinate with RFManager NFC queries
    ensureWifiConnected();
}

void WebManager::unlock() {
    webAccessLocked = false;
    Logger::info("[WebManager] Config unlocked via NFC");
}

void WebManager::lock() {
    webAccessLocked = true;
    Logger::info("[WebManager] Config locked");
}

String WebManager::scanWiFi() {
    Logger::info("[WiFi] Scanning networks...");
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, true);
    if (n < 0) {
        Logger::error("[WiFi] Scan failed: " + String(n));
        return "[]";
    }
    Logger::info("[WiFi] Scan found " + String(n) + " networks");
    String json = "[";
    for (int i = 0; i < n; i++) {
        const String ssid = WiFi.SSID(i);
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + jsonEscape(ssid) + "\"";
        json += ",\"rssi\":" + String(WiFi.RSSI(i));
        json += ",\"enc\":" + String(WiFi.encryptionType(i));
        json += ",\"ch\":" + String(WiFi.channel(i));
        json += "}";
        Logger::info("  " + String(i + 1) + ". " + (ssid.length() ? ssid : String("<hidden>")) +
                     "  RSSI:" + String(WiFi.RSSI(i)) +
                     "  CH:" + String(WiFi.channel(i)) +
                     "  Enc:" + String(WiFi.encryptionType(i)));
    }
    json += "]";
    Logger::debug("[WiFi] SSID list JSON: " + json);
    WiFi.scanDelete();
    return json;
}




