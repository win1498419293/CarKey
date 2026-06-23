#include "DisplayManager.h"
#include "StatusLight.h"
#include "Logger.h"

Adafruit_SH1106G* DisplayManager::_display = nullptr;
bool DisplayManager::_ready = false;
String DisplayManager::_lastL1 = "";
String DisplayManager::_lastL2 = "";
unsigned long DisplayManager::_lastUpdateMs = 0;

DisplayManager displayManager;

void DisplayManager::init() {
    Logger::info("[Display] Initializing SH1106 OLED (SDA=8, SCL=21)...");

    // Power-on stabilization delay (OLED needs time after cold boot)
    delay(300);

    for (int attempt = 1; attempt <= 3; attempt++) {
        // Reset I2C bus to clean state
        Wire.end();
        delay(50);
        Wire.begin(SDA, SCL);
        delay(50);

        _display = new Adafruit_SH1106G(128, 64, &Wire, -1);
        if (_display->begin(ADDR, true)) {
            delay(10);  // Let display stabilize after soft-reset

            // Full SH1106 init sequence (SSD1306-compatible)
            // Many SH1106 modules need this to properly wake charge pump
            _display->oled_command(0xAE);  // Display OFF
            _display->oled_command(0xD5); _display->oled_command(0x80);
            _display->oled_command(0xA8); _display->oled_command(0x3F);
            _display->oled_command(0xD3); _display->oled_command(0x00);
            _display->oled_command(0x40);
            _display->oled_command(0x8D); _display->oled_command(0x14);  // Charge pump
            _display->oled_command(0x20); _display->oled_command(0x00);
            _display->oled_command(0xA1);
            _display->oled_command(0xC8);
            _display->oled_command(0xDA); _display->oled_command(0x12);
            _display->oled_command(0x81); _display->oled_command(0xCF);
            _display->oled_command(0xD9); _display->oled_command(0xF1);
            _display->oled_command(0xDB); _display->oled_command(0x40);
            _display->oled_command(0xA4);
            _display->oled_command(0xA6);
            _display->oled_command(0xAF);  // Display ON
            delay(50);

            _display->clearDisplay();
            _display->setTextColor(SH110X_WHITE);
            _display->setTextSize(2);
            _display->setCursor(0, 0);
            _display->println("CarKey V5");
            _display->setTextSize(1);
            _display->setCursor(0, 35);
            _display->println("S3-N16R8 Boot");
            _display->display();

            _ready = true;
            _lastL1 = "CarKey V5";
            _lastL2 = "S3-N16R8 Boot";
            Logger::info("[Display] SH1106 init OK (attempt " + String(attempt) + "/3)");
            return;
        }

        Logger::warn("[Display] SH1106 begin() failed (attempt " + String(attempt) + "/3)");
        delete _display;
        _display = nullptr;
        delay(200);
    }

    _ready = false;
    Logger::error("[Display] SH1106 init FAILED after 3 attempts - OLED will be offline");
}

void DisplayManager::show(const String& line1, const String& line2) {
    if (!_ready) return;
    if (line1 == _lastL1 && line2 == _lastL2) return;

    _display->clearDisplay();
    _display->setTextColor(SH110X_WHITE);
    _display->setTextSize(2);
    _display->setCursor(0, 0);
    _display->println(line1);
    _display->setTextSize(1);
    _display->setCursor(0, 35);
    _display->println(line2);
    _display->display();

    _lastL1 = line1;
    _lastL2 = line2;
}

void DisplayManager::update() {
    if (!_ready) return;

    // Update every 500ms to avoid I2C congestion
    unsigned long now = millis();
    if (now - _lastUpdateMs < 500) return;
    _lastUpdateMs = now;

    // Map StatusLight state to display messages
    switch (StatusLight::currentState()) {
        case StatusLight::State::OFF:
        case StatusLight::State::IDLE:
            show("CarKey V5", "Ready");
            break;
        case StatusLight::State::NFC_WAITING:
            show("NFC", "Waiting Card...");
            break;
        case StatusLight::State::NFC_AUTHENTICATING:
            show("NFC", "Authenticating...");
            break;
        case StatusLight::State::NFC_SUCCESS:
            show("PASS", "Access Granted");
            break;
        case StatusLight::State::NFC_FAIL:
            show("FAIL", "Access Denied");
            break;
        case StatusLight::State::BLE_SCANNING:
            show("BLE", "Scanning...");
            break;
        case StatusLight::State::BLE_CONNECTED:
            show("BLE", "Connected");
            break;
        case StatusLight::State::BLE_PAIRING:
            show("BLE", "Pairing Mode");
            break;
        case StatusLight::State::OTA:
            show("OTA", "Updating...");
            break;
        case StatusLight::State::ERROR:
            show("ERROR", "System Fault");
            break;
        case StatusLight::State::LOW_BATTERY:
            show("BATTERY", "Low Voltage!");
            break;
    }
}
