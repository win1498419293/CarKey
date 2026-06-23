#include "Config.h"
#include "Logger.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>

Preferences prefs;


String wifi_ssid = "AK47";
String wifi_pass = "zop963.Z";
String bt_name = "REDMI Turbo 4 Pro";
String start_pwd = "123456";

void loadSystemConfig() {
    prefs.begin("carkey_sys", true);

    pinMode(0, INPUT_PULLUP);
    if (digitalRead(0) == LOW) {
        prefs.end();
        prefs.begin("carkey_sys", false);
        prefs.remove("ssid");
        prefs.remove("pass");
        prefs.end();
        prefs.begin("carkey_sys", true);
        Logger::info("[Config] WiFi settings reset to default");
    }
    wifi_ssid = prefs.getString("ssid", "AK47");
    wifi_pass = prefs.getString("pass", "zop963.Z");
    bt_name = prefs.getString("bt_name", "REDMI Turbo 4 Pro");
    start_pwd = prefs.getString("start_pwd", "123456");
    
    if (wifi_ssid.length() == 0) wifi_ssid = "AK47";
    if (wifi_pass.length() == 0) wifi_pass = "zop963.Z";
    if (bt_name.length() == 0) bt_name = "REDMI Turbo 4 Pro";
    if (start_pwd.length() == 0) start_pwd = "123456";

    secAuthEnabled = prefs.getBool("sec_auth", true);
    bleScanEnabled = prefs.getBool("ble_scan", true);
    authMethodNFC = prefs.getBool("auth_nfc", true);
    Logger::info("[Config] Loaded auth_nfc: " + String(authMethodNFC ? "true" : "false"));
    authMethodBLE = prefs.getBool("auth_ble", true);
    authMethodBLE = bleScanEnabled;
    prefs.end();
    Logger::info("--- config loaded ---");
}

bool saveSystemConfig() {
    if (!prefs.begin("carkey_sys", false)) {
        Logger::error("[Config] Failed to open NVS namespace carkey_sys for write");
        return false;
    }

    bool ok = true;
    ok = ok && (prefs.putString("ssid", wifi_ssid) > 0);
    ok = ok && (prefs.putString("pass", wifi_pass) > 0);
    ok = ok && (prefs.putString("bt_name", bt_name) > 0);
    ok = ok && (prefs.putString("start_pwd", start_pwd) > 0);
    ok = ok && prefs.putBool("sec_auth", secAuthEnabled);
    ok = ok && prefs.putBool("ble_scan", bleScanEnabled);
    Logger::info("[Config] Saving auth_nfc: " + String(authMethodNFC ? "true" : "false"));
    ok = ok && prefs.putBool("auth_nfc", authMethodNFC);
    ok = ok && prefs.putBool("auth_ble", bleScanEnabled);
    prefs.end();

    if (!ok) {
        Logger::error("[Config] Failed to persist one or more settings to NVS");
        return false;
    }

    Logger::info("[Config] Settings saved to NVS (nfc=" + String(authMethodNFC ? "ON" : "OFF") + ")");
    return ok;
}

bool nfcUnlockEnabled = true;
bool webAccessLocked = false;
bool bleScanEnabled = true;
bool secAuthEnabled = true;
bool authMethodNFC = true;
bool authMethodBLE = true;

#ifdef CONFIG_IDF_TARGET_ESP32S3
// S3: GPIO22-25 not available, OPI PSRAM on GPIO26,30-38
extern const int PIN_HANDBRAKE = 41;
extern const int PIN_NEUTRAL = 42;
extern const int PIN_IGN = 6;
extern const int PIN_START = 40;
extern const int PIN_HORN = 7;
#else
// ESP32: legacy pins
extern const int PIN_HANDBRAKE = 32;
extern const int PIN_NEUTRAL = 33;
extern const int PIN_IGN = 25;
extern const int PIN_START = 26;
extern const int PIN_HORN = 27;
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3
const int PIN_BATTERY_ADC = 4;   // S3: GPIO4 ADC1_CH3 (GPIO10 used by PN532_SS)
#else
const int PIN_BATTERY_ADC = 36;  // ESP32: original pin
#endif
const float BATTERY_VOLTAGE_MULTIPLIER = 7.8f;
const float BATTERY_LOW_START_BLOCK_V = 11.8f;
const unsigned long START_CHARGE_VERIFY_TIMEOUT_MS = 10000;
const float BATTERY_ALTERNATOR_MIN_V = 13.5f;

#if ENABLE_BLE
BlePeer allowedDevices[BLE_DEVICE_COUNT] = {
    {"REDMI Turbo 4 Pro", "", -85},
};
#endif


String webLogBuffer = "";
static portMUX_TYPE g_logMux = portMUX_INITIALIZER_UNLOCKED;

void sysLog(String msg, String type) {
    Serial.println("[" + type + "] " + msg);
    taskENTER_CRITICAL(&g_logMux);
    webLogBuffer += type + "|" + msg + "\n";
    if (webLogBuffer.length() > 2000) {
        webLogBuffer = webLogBuffer.substring(1000);
    }
    taskEXIT_CRITICAL(&g_logMux);
}

String takeWebLogBuffer() {
    taskENTER_CRITICAL(&g_logMux);
    String out = webLogBuffer;
    webLogBuffer = "";
    taskEXIT_CRITICAL(&g_logMux);
    return out;
}
volatile bool g_nfcScanning = false;
