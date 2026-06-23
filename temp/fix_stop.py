import re

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

old_start = content.find("void BLEManager::stopPairing()")
old_end = content.find("bool BLEManager::isPairing()")
old_func = content[old_start:old_end]

new_func = """void BLEManager::stopPairing() {
    if (!pairingActive) return;
    Logger::info("[BLE Pair] Stopping pairing mode");
    if (pAdvertising) { pAdvertising->stop(); pAdvertising = nullptr; }
    if (pService) { pService->stop(); pService = nullptr; }
    if (pServer) { pServer = nullptr; }
    pairingActive = false;
    // 恢复 BLE 扫描模式
    BLEDevice::deinit(false);
    delay(200);
    // 重新初始化扫描
    Logger::info("[BLE Pair] Reinitializing BLE scanner");
    BLEDevice::init(bt_name.c_str());
    BLEDevice::setPower(ESP_PWR_LVL_N12);
    pBLEScan = BLEDevice::getScan();
    if (pBLEScan) {
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
        pBLEScan->setActiveScan(false);
        applyScanProfile(true);
        startScanCycle();
    }
    // 恢复 WiFi
    if (wifi_ssid.length() > 0) {
        Logger::info("[BLE Pair] Reconnecting WiFi");
        WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    }
}

"""

content = content[:old_start] + new_func + content[old_end:]

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("stopPairing updated with scanner reinit")
