#include "DisplayManager.h"
#include "Logger.h"
#include "VehicleStatus.h"
#include "BatteryVoltage.h"
#include "Config.h"
#include <WiFi.h>
#include "BLEManager.h"
#if ENABLE_CELLULAR
#include "NetworkManager.h"
#endif

// Shared auth timestamps (set by TaskManager/NFC)
unsigned long g_lastNfcAuthMs = 0;
constexpr unsigned long kAuthWindowMs = 60000;  // 60s auth window

Adafruit_SH1106G* DisplayManager::_display = nullptr;
bool DisplayManager::_ready = false;
String DisplayManager::_lastL1 = "";
String DisplayManager::_lastL2 = "";
unsigned long DisplayManager::_lastUpdateMs = 0;
uint32_t DisplayManager::_lastHash = 0;

DisplayManager displayManager;

void DisplayManager::init() {
    Logger::info("[Display] Initializing SH1106 OLED (SDA=8, SCL=21)...");
    delay(300);
    for (int attempt = 1; attempt <= 3; attempt++) {
        Wire.end();
        delay(50);
        Wire.begin(SDA, SCL);
        delay(50);
        _display = new Adafruit_SH1106G(128, 64, &Wire, -1);
        if (_display->begin(ADDR, true)) {
            delay(10);
            _display->oled_command(0xAE);
            _display->oled_command(0xD5); _display->oled_command(0x80);
            _display->oled_command(0xA8); _display->oled_command(0x3F);
            _display->oled_command(0xD3); _display->oled_command(0x00);
            _display->oled_command(0x40);
            _display->oled_command(0x8D); _display->oled_command(0x14);
            _display->oled_command(0x20); _display->oled_command(0x00);
            _display->oled_command(0xA1);
            _display->oled_command(0xC8);
            _display->oled_command(0xDA); _display->oled_command(0x12);
            _display->oled_command(0x81); _display->oled_command(0xCF);
            _display->oled_command(0xD9); _display->oled_command(0xF1);
            _display->oled_command(0xDB); _display->oled_command(0x40);
            _display->oled_command(0xA4);
            _display->oled_command(0xA6);
            _display->oled_command(0xAF);
            delay(50);
            _ready = true;
            buildStatusPage();
            Logger::info("[Display] SH1106 init OK (attempt " + String(attempt) + "/3)");
            return;
        }
        Logger::warn("[Display] SH1106 begin() failed (attempt " + String(attempt) + "/3)");
        delete _display;
        _display = nullptr;
        delay(200);
    }
    _ready = false;
    Logger::error("[Display] SH1106 init FAILED after 3 attempts");
}

void DisplayManager::buildStatusPage() {
    if (!_ready) return;

    String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : (const char*)"No WiFi";
    float bv = batteryVoltage.hasReading() ? batteryVoltage.getVoltage() : 0.0f;
    bool bleAuth = bleManager.isAuthorizedDeviceConnected();
    bool nfcAuth = (g_lastNfcAuthMs > 0) && (millis() - g_lastNfcAuthMs < kAuthWindowMs);

#if ENABLE_CELLULAR
    bool cellOnline = networkManager.isCellularOnline();
#else
    bool cellOnline = false;
#endif

    uint32_t hash = (uint32_t)(bleAuth) * 100000 + (uint32_t)(nfcAuth) * 1000 +
                    (uint32_t)(cellOnline) * 100 + (uint32_t)(bv * 100);
    for (int i = 0; i < ip.length(); i++) hash = hash * 31 + ip[i];
    if (hash == _lastHash) return;
    _lastHash = hash;

    _display->clearDisplay();
    _display->setTextColor(SH110X_WHITE);
    _display->setTextSize(1);

    _display->setCursor(0, 2);
    _display->print("IP: "); _display->print(ip);

    _display->setCursor(0, 14);
    _display->print("4G: "); _display->print(cellOnline ? "Online" : "Offline");

    _display->setCursor(0, 26);
    _display->print("BLE: "); _display->print(bleAuth ? "Auth OK" : "No Auth");

    _display->setCursor(0, 38);
    _display->print("NFC: ");
    if (nfcAuth) {
        unsigned long remain = (kAuthWindowMs - (millis() - g_lastNfcAuthMs)) / 1000;
        _display->print("Auth OK ");
        _display->print(remain);
        _display->print("s");
    } else {
        _display->print("No Auth");
    }

    _display->setCursor(0, 50);
    _display->print("BAT: "); _display->print(bv, 1); _display->print("V");

    _display->display();
    _lastL1 = ip;
    _lastL2 = "";
}

void DisplayManager::show(const String& line1, const String& line2) {}
void DisplayManager::update() {
    if (!_ready) return;
    if (millis() - _lastUpdateMs < 500) return;
    _lastUpdateMs = millis();
    buildStatusPage();
}
