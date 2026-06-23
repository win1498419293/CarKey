#pragma once
#include <Arduino.h>

// ===== Pins =====
#define SDA_PIN 21
#define SCL_PIN 22
#define RELAY_PIN 5

// ===== PN532 SPI pins =====
#ifdef CONFIG_IDF_TARGET_ESP32S3
// S3: actual wiring
#define PN532_SCK  12
#define PN532_MISO 13
#define PN532_MOSI 11
#define PN532_SS   10
#else
#define PN532_SCK   12
#define PN532_MISO  13
#define PN532_MOSI  11
#define PN532_SS    10
#endif

// ===== Feature switches =====
#define ENABLE_NFC true
#define ENABLE_BLE true
#define ENABLE_CELLULAR false

// ===== NFC parameters =====
#define NFC_COOLDOWN 1500
#define NFC_I2C_CLOCK_HZ 10000U
#define NFC_SCAN_INTERVAL_MS 120U
#define NFC_SCAN_WINDOW_MS 110U
#define NFC_SCAN_SETTLE_MS 4U
#define NFC_SCAN_POLL_TIMEOUT_MS 40U
#define NFC_SCAN_RETRY_DELAY_MS 2U
#define NFC_SAME_UID_COOLDOWN_MS 5000U

// ===== BLE parameters =====
#define BLE_POST_AUTH_COOLDOWN_MS (5UL * 60UL * 60UL * 1000UL)

// --- Vehicle state sensor pins ---
extern const int PIN_HANDBRAKE;
extern const int PIN_NEUTRAL;
extern const int PIN_IGN;
extern const int PIN_START;
extern const int PIN_HORN;
extern const int PIN_BATTERY_ADC;
extern const float BATTERY_VOLTAGE_MULTIPLIER;
extern const float BATTERY_LOW_START_BLOCK_V;
extern const unsigned long START_CHARGE_VERIFY_TIMEOUT_MS;
extern const float BATTERY_ALTERNATOR_MIN_V;

// ===== Configurable system settings =====
extern String wifi_ssid;
extern String wifi_pass;
extern String bt_name;
extern String start_pwd;

// --- System security config ---
extern bool secAuthEnabled;
extern bool authMethodNFC;
extern bool authMethodBLE;
extern bool webAccessLocked;
extern bool bleScanEnabled;
extern unsigned long watchdogWarnMs;
extern unsigned long watchdogSoftRecoverMs;
extern unsigned long watchdogHardRecoverMs;

// --- Storage management ---
void loadSystemConfig();
bool saveSystemConfig();

// ===== BLE =====
#if ENABLE_BLE
// BLE authorized device list
struct BlePeer {
    const char* name;
    const char* mac;
    int rssi;
};

#define BLE_DEVICE_COUNT 1
extern BlePeer allowedDevices[BLE_DEVICE_COUNT];
#endif

// --- Log buffer ---
extern String webLogBuffer;
void sysLog(String msg, String type = "info");
String takeWebLogBuffer();


// --- Build stamp ---
extern const char* const kBuildStamp;
// --- NFC whitelist ---
extern volatile bool g_nfcScanning;


