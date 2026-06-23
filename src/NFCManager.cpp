#include "NFCManager.h"
#include "Config.h"
#include "Logger.h"
#include <Arduino.h>
#include <string.h>

NFCManager::NFCManager() : nfc(PN532_SS) {}

void NFCManager::init() {
    Logger::info("[NFC] Initializing PN532...");

    for (int attempt = 1; attempt <= 3; attempt++) {
        SPI.end();
        delay(50);
        SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
        nfc.begin();

        uint32_t version = nfc.getFirmwareVersion();
        if (!version) {
            Logger::error("PN532 not found (attempt " + String(attempt) + "/3)");
            if (attempt < 3) continue;
            initFailed = true;
            return;
        }

        if (!nfc.SAMConfig()) {
            Logger::error("PN532 SAMConfig failed (attempt " + String(attempt) + "/3)");
            if (attempt < 3) continue;
            initFailed = true;
            return;
        }

        Logger::info("PN532 init success (attempt " + String(attempt) + "/3)");
        initFailed = false;
        lastPollMs = millis();
        return;
    }
}

bool NFCManager::readCard(uint8_t* uid, uint8_t& len) {
    bool ok = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, 1000);
    if (ok) {
        lastPollMs = millis();
    }
    return ok;
}

bool NFCManager::pollCard(uint8_t* uid, uint8_t& len, uint16_t timeoutMs) {
    bool ok = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, timeoutMs);
    if (ok) {
        lastPollMs = millis();
    }
    return ok;
}

bool NFCManager::isHealthy() {
    uint32_t version = nfc.getFirmwareVersion();
    return (version != 0);
}

void NFCManager::reconfig() {
    nfc.SAMConfig();
}

bool NFCManager::checkCard() {
    uint8_t uid[7];
    uint8_t len = 0;
    return readCard(uid, len);
}
