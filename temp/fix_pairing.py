import re

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Replace startPairing with a version that does full BLE reinit
old_start = content.find("bool BLEManager::startPairing()")
old_end = content.find("void BLEManager::stopPairing()")
old_func = content[old_start:old_end]

new_func = """bool BLEManager::startPairing() {
    if (pairingActive) return true;
    Logger::info("[BLE Pair] Starting pairing mode...");
    stopActiveScan();
    if (WiFi.status() == WL_CONNECTED) {
        Logger::info("[BLE Pair] Disconnecting WiFi for BLE radio");
        WiFi.disconnect(true);
    }
    // 完全重置 BLE 栈，切换到外设模式
    BLEDevice::deinit(false);
    delay(300);
    BLEDevice::init(bt_name.c_str());
    BLEDevice::setPower(ESP_PWR_LVL_P7);  // 提高功率便于发现
    delay(200);
    pairDone = false;
    pServer = BLEDevice::createServer();
    if (!pServer) { Logger::error("[BLE Pair] Server create failed"); return false; }
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
    Logger::info("[BLE Pair] Advertising as: " + bt_name + " (enable GPS on Android!)");
    return true;
}

"""

content = content[:old_start] + new_func + content[old_end:]

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("startPairing rewritten with full BLE reinit")
