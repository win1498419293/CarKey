#include "Config.h"
#include "RFManager.h"
#include <esp_wifi.h>
#include <WiFi.h>
#if ENABLE_BLE
#include "BLEManager.h"

#include "Config.h"
#include "StatusLight.h"
#include "Logger.h"
#include "Metrics.h"

#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_gap_ble_api.h>
#include <Preferences.h>

namespace {

bool g_pendingNameDiscovery = false;
String g_discoveredName;
int g_discoveredRSSI = -128;
unsigned long g_discoveryStartMs = 0;

BLEScan* pBLEScan = nullptr;

constexpr uint32_t kFastScanDurationSeconds = 3;
constexpr uint32_t kEcoScanDurationSeconds = 3;
constexpr uint32_t kFastScanRestartDelayMs = 3000;
constexpr uint32_t kEcoScanRestartDelayMs = 5000;
constexpr uint32_t kScanHangTimeoutMs = 9000;
constexpr uint32_t kFastScanWindowMs = 30;
constexpr uint32_t kFastScanIntervalMs = 100;
constexpr uint32_t kEcoScanWindowMs = 30;
constexpr uint32_t kEcoScanIntervalMs = 160;
constexpr uint32_t kFastProfileAfterAuthMs = 120000;
constexpr uint32_t kFastProfileOnBootMs = 10000;
constexpr uint32_t kScanRestartBackoffStepMs = 250;
constexpr uint32_t kScanRestartBackoffMaxMs = 3000;
constexpr uint32_t kAuthOnlineWindowMs = 180000;

constexpr uint32_t kPairingTimeoutMs = 60000;
const char* kPairServiceUUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const char* kPairCharUUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

String normalizeMac(const String& src) {
    String out; out.reserve(src.length());
    for (size_t i = 0; i < src.length(); i++) { const char c = src[i]; if ((c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f')) out += static_cast<char>(toupper(c)); }
    return out;
}

String bdaToString(const uint8_t* bda) { char buf[18]; snprintf(buf,sizeof(buf),"%02X:%02X:%02X:%02X:%02X:%02X",bda[0],bda[1],bda[2],bda[3],bda[4],bda[5]); return String(buf); }

void handleBleScanComplete(BLEScanResults results) { bleManager.onScanResults(results.getCount()); bleManager.onScanComplete(); }

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        static unsigned long lastLogMs = 0;
        const auto now = millis();
        if (now - lastLogMs > 3000) {
            lastLogMs = now;
        }

        String deviceMac = normalizeMac(advertisedDevice.getAddress().toString().c_str());
        String deviceName = advertisedDevice.getName().c_str();
        deviceName.toUpperCase();
        const int rssi = advertisedDevice.getRSSI();

        if (g_pendingNameDiscovery && deviceName.length() > 0 && rssi > g_discoveredRSSI) {
            g_discoveredName = advertisedDevice.getName().c_str();
            g_discoveredRSSI = rssi;
        }

        if (bleManager.isPairedDevice(deviceMac.c_str(), deviceName.c_str()) && rssi >= -85) {
            bleManager.onAuthorizedHit(millis(), bleManager.getPairedDeviceName());
            Metrics::onBleAuthorizedDetected();
            return;
        }

        for (int i = 0; i < BLE_DEVICE_COUNT; i++) {
            String allowedMac = normalizeMac(String(allowedDevices[i].mac));
            String allowedName = String(allowedDevices[i].name);
            allowedName.toUpperCase();
            if (((allowedMac.length() > 0 && deviceMac == allowedMac) ||
                 (allowedName.length() > 0 && deviceName.length() > 0 && deviceName == allowedName)) &&
                rssi >= allowedDevices[i].rssi) {
                bleManager.onAuthorizedHit(millis(), allowedDevices[i].name);
                Metrics::onBleAuthorizedDetected();
                return;
            }
        }
    }
};

class PairServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pSrv, esp_ble_gatts_cb_param_t* param) override {
        String mac = bdaToString(param->connect.remote_bda);
        Logger::info("[BLE Pair] Phone connected: " + mac);
        bleManager.savePairedDevice(mac.c_str(), "");
    }
    void onDisconnect(BLEServer* pSrv) override {
        Logger::info("[BLE Pair] Phone disconnected");
    }
};

}  // namespace

BLEManager bleManager;

void BLEManager::savePairedDevice(const char* mac, const char* name) {
    strncpy(pairedDeviceMac, mac, sizeof(pairedDeviceMac) - 1);
    if (name && name[0]) {
        strncpy(pairedDeviceName, name, sizeof(pairedDeviceName) - 1);
    } else {
        int ml = strlen(mac);
        snprintf(pairedDeviceName, sizeof(pairedDeviceName), "Phone-%s", ml > 4 ? mac + ml - 4 : mac);
    }

    devicePaired = true;
    pairDone = true;

    if (!name || !name[0]) {
        g_pendingNameDiscovery = true;
        g_discoveredName = "";
        g_discoveredRSSI = -128;
        g_discoveryStartMs = millis();
        Logger::info("[BLE Pair] Will discover phone name via scan...");
    }

    Logger::info(String("[BLE Pair] Paired: ") + pairedDeviceName + " (" + pairedDeviceMac + ")");
    Preferences prefs;
    if (prefs.begin("carkey_bt", false)) {
        prefs.putString("p_name", pairedDeviceName);
        prefs.putString("p_mac", pairedDeviceMac);
        prefs.end();
    }
}

void BLEManager::loadPairedDevice() {
    Preferences prefs;
    if (prefs.begin("carkey_bt", true)) {
        String n = prefs.getString("p_name", "");
        String m = prefs.getString("p_mac", "");
        strncpy(pairedDeviceName, n.c_str(), sizeof(pairedDeviceName) - 1);
        strncpy(pairedDeviceMac, m.c_str(), sizeof(pairedDeviceMac) - 1);
        prefs.end();
    }
    devicePaired = (pairedDeviceName[0] != 0 || pairedDeviceMac[0] != 0);
    if (devicePaired) Logger::info(String("[BLE Pair] Loaded: ") + pairedDeviceName + " (" + pairedDeviceMac + ")");
}

bool BLEManager::isPairedDevice(const char* mac, const char* name) const {
    if (!devicePaired) return false;

    // Name-only matching (phones use random MAC, so MAC is unreliable)
    String pn = String(pairedDeviceName);
    pn.toUpperCase();
    String dn = String(name ? name : "");
    dn.toUpperCase();
    if (pn.length() > 0 && dn.length() > 0 && dn.indexOf(pn) >= 0) {
        return true;
    }

    return false;
}

bool BLEManager::startPairing() {
    if (pairingActive) return true;
    Logger::info("[BLE Pair] Starting pairing mode...");
    stopActiveScan();
    delay(300);
    pairDone = false;
    pServer = BLEDevice::createServer();
    if (!pServer) { Logger::error("[BLE Pair] Server create failed, resuming scan"); startScanCycle(); return false; }
    pServer->setCallbacks(new PairServerCallbacks());
    pService = pServer->createService(kPairServiceUUID);
    pCharMac = pService->createCharacteristic(kPairCharUUID, BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_NOTIFY);
    pCharMac->setValue("CarKeyV5");
    pService->start();
    pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(kPairServiceUUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinInterval(32);
    pAdvertising->setMaxInterval(64);
    pAdvertising->start();
    pairingStartMs = millis();
    pairingActive = true;
    StatusLight::setBLEPairing();
    Logger::info("[BLE Pair] Advertising as: " + bt_name);
    return true;
}

void BLEManager::stopPairing() {
    if (!pairingActive) return;
    Logger::info("[BLE Pair] Stopping pairing mode");
    if (pAdvertising) { pAdvertising->stop(); pAdvertising = nullptr; }
    if (pService) { pService->stop(); pService = nullptr; }
    if (pServer) { pServer = nullptr; }
    pairingActive = false;
    StatusLight::setIdle();
    delay(300);
    Logger::info("[BLE Pair] Resuming BLE scan");
    startScanCycle();
}

bool BLEManager::isPairing() const { return pairingActive; }
bool BLEManager::isPaired() const { return devicePaired; }
const char* BLEManager::getPairedDeviceName() const { return pairedDeviceName; }
const char* BLEManager::getPairedDeviceMac() const { return pairedDeviceMac; }

void BLEManager::clearPairing() {
    Logger::info("[BLE Pair] Clearing paired device");
    devicePaired = false; pairedDeviceName[0]=0; pairedDeviceMac[0]=0;
    Preferences prefs; if (prefs.begin("carkey_bt", false)) { prefs.remove("p_name"); prefs.remove("p_mac"); prefs.end(); }
}

// ================== Public API ==================

void BLEManager::onScanResults(int deviceCount) { scanDeviceCount = deviceCount; lastScanResultTime = millis(); }
void BLEManager::onAuthorizedHit(unsigned long atMs, const char* name) { lastAuthorizedTime=atMs; lastAuthDetectedAt=atMs; }

void BLEManager::onBleAuthSuccess() {
    postAuthCooldownUntilMs = millis() + BLE_POST_AUTH_COOLDOWN_MS;
    stopActiveScan();
    Logger::info("[BLE] auth success, scan paused for 5 hours");
}

bool BLEManager::isReady() const {
    return pBLEScan != nullptr;
}

void BLEManager::init() {
    if (!bleScanEnabled) {
        if (isScanning) {
            stopActiveScan();
        }
        Logger::info("[BLE] scan disabled, skip init");
        return;
    }

    if (!authMethodBLE) {
        if (isScanning) {
            stopActiveScan();
        }
        Logger::info("[BLE] auth method disabled, skip init");
        return;
    }

    if (pBLEScan != nullptr) {
        if (!isScanning && !pairingActive && !isScanCooldownActive()) {
            startScanCycle();
        }
        return;
    }

    // Start BLE scan - configure device name and scan params
    Logger::info("[BLE] init scanning guard");
    BLEDevice::init(bt_name.c_str());
    BLEDevice::setPower(ESP_PWR_LVL_N12);
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM); Logger::info(String("[BLE] BLEDevice init name=")+bt_name);
    pBLEScan = BLEDevice::getScan(); if (pBLEScan == nullptr) { Logger::error("[BLE] getScan() returned NULL"); return; }
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->setActiveScan(false); applyScanProfile(true);
    if (startScanCycle()) Logger::info("[BLE] scan loop started");
}
void BLEManager::applyScanProfile(bool fastProfile) { if(!pBLEScan)return; activeFastProfile=fastProfile; pBLEScan->setInterval(fastProfile?kFastScanIntervalMs:kEcoScanIntervalMs); pBLEScan->setWindow(fastProfile?kFastScanWindowMs:kEcoScanWindowMs); }
bool BLEManager::shouldUseFastProfile(unsigned long now) const {
    if (RFManager::isNfcActive()) {
        return false;
    }

    if (authMethodNFC && WiFi.status() == WL_CONNECTED) {
        return false;
    }

    return now < kFastProfileOnBootMs || (lastAuthDetectedAt != 0 && (now - lastAuthDetectedAt) < kFastProfileAfterAuthMs);
}
bool BLEManager::startScanCycle() {
    if (!pBLEScan||isScanning||pairingActive) return false;
    if (isScanCooldownActive()) return false;
    unsigned long now=millis(); bool sf=shouldUseFastProfile(now); if(sf!=activeFastProfile)applyScanProfile(sf);
    pBLEScan->clearResults();
    if(!pBLEScan->start(activeFastProfile?kFastScanDurationSeconds:kEcoScanDurationSeconds,handleBleScanComplete,false)) {
        lastScanEndTime=now; scanStartFailStreak++; nextScanEarliestAt=now+min((uint32_t)scanStartFailStreak*kScanRestartBackoffStepMs,kScanRestartBackoffMaxMs); return false;
    }
    scanStartFailStreak=0; nextScanEarliestAt=0; lastScanStartTime=now; isScanning=true; Metrics::onBleScanStart(); return true;
}
void BLEManager::stopScanCycle(const char* r) { if(isScanning&&pBLEScan){pBLEScan->stop();pBLEScan->clearResults();isScanning=false;lastScanEndTime=millis();} }
void BLEManager::onScanComplete() {
    isScanning = false;
    lastScanEndTime = millis();
    if (pBLEScan) pBLEScan->clearResults();

    if (g_pendingNameDiscovery && g_discoveredName.length() > 0) {
        strncpy(pairedDeviceName, g_discoveredName.c_str(), sizeof(pairedDeviceName) - 1);
        Logger::info(String("[BLE Pair] Discovered phone name: ") + pairedDeviceName);
        Preferences prefs;
        if (prefs.begin("carkey_bt", false)) {
            prefs.putString("p_name", pairedDeviceName);
            prefs.end();
        }
        g_pendingNameDiscovery = false;
        Logger::info("[BLE Pair] Name discovery complete, ready for name-based matching");
    }

    if (g_pendingNameDiscovery && (millis() - g_discoveryStartMs > 10000)) {
        g_pendingNameDiscovery = false;
        Logger::warn("[BLE Pair] Name discovery timeout, using fallback name");
    }
}

void BLEManager::update() {
    static bool _loaded = false;
    if (!_loaded) {
        loadPairedDevice();
        _loaded = true;
    }

    if (pairingActive) {
        if (pairDone) {
            stopPairing();
            return;
        }
        if (millis() - pairingStartMs > kPairingTimeoutMs) {
            Logger::warn("[BLE Pair] Timeout");
            stopPairing();
        }
        return;
    }

    if (!bleScanEnabled || !authMethodBLE || !pBLEScan) {
        if (isScanning) {
            stopActiveScan();
        }
        return;
    }

    const unsigned long now = millis();
    if (isScanCooldownActive()) {
        if (isScanning) {
            stopActiveScan();
        }
        return;
    }

    if (isScanning) {
        if (now - lastScanStartTime > kScanHangTimeoutMs) {
            stopScanCycle("hung");
            scanStartFailStreak++;
        }
        return;
    }

    const uint32_t d = activeFastProfile ? kFastScanRestartDelayMs : (WiFi.status() == WL_CONNECTED ? 30000 : kEcoScanRestartDelayMs);
    if (lastScanEndTime && now - lastScanEndTime < d) return;
    if (nextScanEarliestAt && static_cast<long>(now - nextScanEarliestAt) < 0) return;
    startScanCycle();
}

void BLEManager::stopActiveScan() { if(isScanning&&pBLEScan){pBLEScan->stop();pBLEScan->clearResults();isScanning=false;lastScanEndTime=millis();} }

bool BLEManager::isScanCooldownActive() const {
    return postAuthCooldownUntilMs != 0 && static_cast<long>(millis() - postAuthCooldownUntilMs) < 0;
}

unsigned long BLEManager::getScanCooldownRemainingMs() const {
    if (!isScanCooldownActive()) return 0;
    return postAuthCooldownUntilMs - millis();
}

bool BLEManager::isAuthorizedDeviceConnected() {
    if (!authMethodBLE || !bleScanEnabled || isScanCooldownActive()) return false;
    if (lastAuthorizedTime == 0) return false;
    return millis() - lastAuthorizedTime < kAuthOnlineWindowMs;
}
bool BLEManager::isScanningBLE() const { return isScanning; }
int BLEManager::getLastSeenSec() const { return lastAuthorizedTime?static_cast<int>((millis()-lastAuthorizedTime)/1000):-1; }
int BLEManager::getScanDeviceCount() const { return scanDeviceCount; }
unsigned long BLEManager::getLastScanTime() const { return lastScanResultTime; }

#endif
