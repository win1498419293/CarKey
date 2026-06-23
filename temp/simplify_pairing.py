with open(r"D:\CarKey_V5\src\BLEManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Replace startPairing - no deinit, just stop scan and start advertising
old_start = content.find("bool BLEManager::startPairing()")
old_end = content.find("void BLEManager::stopPairing()")
old_func = content[old_start:old_end]

new_func = """bool BLEManager::startPairing() {
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
    Logger::info("[BLE Pair] Advertising as: " + bt_name);
    return true;
}

"""

content = content[:old_start] + new_func + content[old_end:]

# Replace stopPairing - no deinit, just stop server and resume scan
old_stop = content.find("void BLEManager::stopPairing()")
old_stop_end = content.find("bool BLEManager::isPairing()")
old_stop_func = content[old_stop:old_stop_end]

new_stop_func = """void BLEManager::stopPairing() {
    if (!pairingActive) return;
    Logger::info("[BLE Pair] Stopping pairing mode");
    if (pAdvertising) { pAdvertising->stop(); pAdvertising = nullptr; }
    if (pService) { pService->stop(); pService = nullptr; }
    if (pServer) { pServer = nullptr; }
    pairingActive = false;
    delay(300);
    Logger::info("[BLE Pair] Resuming BLE scan");
    startScanCycle();
}

"""

content = content[:old_stop] + new_stop_func + content[old_stop_end:]

# Remove the deinit/reinit in stopPairing (the old code might have artifacts)
# Also remove WiFi disconnect/reconnect
content = content.replace(
    "if (WiFi.status() == WL_CONNECTED) {\n        Logger::info(\"[BLE Pair] Disconnecting WiFi for BLE radio\");\n        WiFi.disconnect(true);\n    }\n    // 完全重置 BLE 栈，切换到外设模式",
    ""
)
content = content.replace(
    "BLEDevice::deinit(false);\n    delay(300);\n    BLEDevice::init(bt_name.c_str());\n    BLEDevice::setPower(ESP_PWR_LVL_P7);  // 提高功率便于发现\n    delay(200);\n    ",
    ""
)

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("Simplified pairing - no deinit/reinit")
