import re
with open(r'D:\CarKey_V5\src\BLEManager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find and replace startPairing
pattern = r'bool BLEManager::startPairing\(\).*?(?=void BLEManager::stopPairing)'
replacement = '''bool BLEManager::startPairing() {
    if (pairingActive) return true;
    Logger::info("[BLE Pair] Starting pairing mode...");
    stopActiveScan();
    if (WiFi.status() == WL_CONNECTED) {
        Logger::info("[BLE Pair] Disconnecting WiFi for BLE radio");
        WiFi.disconnect(true);
    }
    pairDone = false;
    esp_ble_gap_set_device_name(bt_name.c_str());
    delay(200);
    pServer = BLEDevice::createServer();
    if (!pServer) { Logger::error("[BLE Pair] Server create failed"); return false; }
    pServer->setCallbacks(new PairServerCallbacks());
    pService = pServer->createService(kPairServiceUUID);
    pCharMac = pService->createCharacteristic(kPairCharUUID, BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_NOTIFY);
    pCharMac->setValue("CarKeyV5");
    pService->start();
    BLEAdvertising* pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(kPairServiceUUID);
    pAdv->setScanResponse(true);
    pAdv->setMinPreferred(0x06);
    pAdv->setMinInterval(32);
    pAdv->setMaxInterval(64);
    pAdv->start();
    pairingStartMs = millis();
    pairingActive = true;
    Logger::info("[BLE Pair] Advertising as: " + bt_name + " (wait 10-30s, enable GPS on Android)");
    return true;
}'''

content = re.sub(pattern, replacement, content, flags=re.DOTALL)

with open(r'D:\CarKey_V5\src\BLEManager.cpp', 'w', encoding='utf-8', newline='') as f:
    f.write(content)
print('startPairing replaced')
