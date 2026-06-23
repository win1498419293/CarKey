#pragma once
#include "RFManager.h"
#include <Adafruit_PN532.h>
#include <SPI.h>

class NFCManager {
public:
    NFCManager();
    void init();
    bool readCard(uint8_t* uid, uint8_t& len);
    bool checkCard();
    bool pollCard(uint8_t* uid, uint8_t& len, uint16_t timeoutMs = 200);
    bool isHealthy();
    bool isInitFailed() const { return initFailed; }
    void reconfig();

private:
    Adafruit_PN532 nfc;
    unsigned long lastPollMs = 0;
    unsigned long lastAcceptedUidMs = 0;
    uint8_t lastAcceptedUid[7] = {0};
    bool initFailed = false;
    uint8_t lastAcceptedUidLen = 0;
};

extern NFCManager nfcManager;