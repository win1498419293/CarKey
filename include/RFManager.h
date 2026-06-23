#pragma once
#include <Arduino.h>
#include <esp_wifi.h>

// RF coordinator for NFC / BLE / WiFi contention.
// NFC gets priority and temporarily pushes WiFi into a lower-interference mode.

enum class RFMode : uint8_t {
    RF_IDLE = 0,
    RF_NFC,
    RF_BLE,
    RF_WIFI
};

class RFManager {
public:
    static RFMode currentMode;
    static bool nfcRequested;
    static int8_t savedWifiTxPower;
    static wifi_ps_type_t savedWifiPsMode;
    static bool savedWifiPsModeValid;

    static void beginNFC() {
        if (currentMode == RFMode::RF_NFC) return;
        nfcRequested = true;

        // Reduce WiFi interference while the PN532 is being polled.
        savedWifiTxPower = getWifiTxPower();
        savedWifiPsModeValid = (esp_wifi_get_ps(&savedWifiPsMode) == ESP_OK);
        setWifiTxPowerMin();
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

        currentMode = RFMode::RF_NFC;
    }

    static void endNFC() {
        if (currentMode != RFMode::RF_NFC) return;
        nfcRequested = false;

        restoreWifiTxPower();
        restoreWifiPsMode();
        currentMode = RFMode::RF_IDLE;
    }

    static bool isNfcActive() {
        return currentMode == RFMode::RF_NFC;
    }

private:
    static int8_t getWifiTxPower() {
        int8_t power = 78;
        esp_wifi_get_max_tx_power(&power);
        return power;
    }

    static void setWifiTxPowerMin() {
        // Lowest supported value on ESP32 Arduino WiFi.
        esp_wifi_set_max_tx_power(2);
    }

    static void restoreWifiTxPower() {
        if (savedWifiTxPower > 0) {
            esp_wifi_set_max_tx_power(savedWifiTxPower);
        }
    }

    static void restoreWifiPsMode() {
        if (savedWifiPsModeValid) {
            esp_wifi_set_ps(savedWifiPsMode);
            savedWifiPsModeValid = false;
        }
    }
};
