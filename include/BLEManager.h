#include "Config.h"
#if ENABLE_BLE
#pragma once

#include <Arduino.h>
#include <BLEDevice.h>

class BLEServer;
class BLEService;
class BLECharacteristic;
class BLEAdvertising;

class BLEManager {
public:
    void init();
    void update();
    bool isAuthorizedDeviceConnected();
    void onScanComplete();
    void onAuthorizedHit(unsigned long atMs, const char* name = nullptr);
    void onBleAuthSuccess();
    void onScanResults(int deviceCount);

    bool isScanningBLE() const;
    bool isReady() const;
    void stopActiveScan();
    bool isScanCooldownActive() const;
    unsigned long getScanCooldownRemainingMs() const;

    int getLastSeenSec() const;
    int getScanDeviceCount() const;
    unsigned long getLastScanTime() const;

    // Public state accessors
    bool startPairing();
    void stopPairing();
    bool isPairing() const;
    bool isPaired() const;
    const char* getPairedDeviceName() const;
    const char* getPairedDeviceMac() const;
    void clearPairing();
    void savePairedDevice(const char* mac, const char* name);
    void loadPairedDevice();
    bool isPairedDevice(const char* mac, const char* name) const;

    unsigned long lastAuthorizedTime = 0;

private:
    bool startScanCycle();
    void stopScanCycle(const char* reason);
    void applyScanProfile(bool fastProfile);
    bool shouldUseFastProfile(unsigned long now) const;

    volatile bool isScanning = false;
    volatile unsigned long lastScanStartTime = 0;
    volatile unsigned long lastScanEndTime = 0;
    volatile unsigned long nextScanEarliestAt = 0;
    volatile unsigned long lastAuthDetectedAt = 0;
    volatile unsigned long postAuthCooldownUntilMs = 0;
    uint8_t scanStartFailStreak = 0;
    volatile int scanDeviceCount = 0;
    volatile unsigned long lastScanResultTime = 0;
    bool activeFastProfile = true;

    // Private helpers (char arrays, not String)
    bool pairingActive = false;
    bool devicePaired = false;
    char pairedDeviceName[32] = {0};
    char pairedDeviceMac[18] = {0};
    unsigned long pairingStartMs = 0;
    bool pairDone = false;
    BLEServer* pServer = nullptr;
    BLEService* pService = nullptr;
    BLECharacteristic* pCharMac = nullptr;
    BLEAdvertising* pAdvertising = nullptr;
};

extern BLEManager bleManager;

#endif
