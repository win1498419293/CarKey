with open(r"D:\CarKey_V5\src\BLEManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Remove security includes
content = content.replace('#include <BLESecurity.h>\n', '')

# Remove MySecurityCallbacks class
old_class = """// BLE 安全回调 - Just Works 配对（无需 PIN）
class MySecurityCallbacks : public BLESecurityCallbacks {
    bool onSecurityRequest() override { return true; }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth) override {
        if (auth.success) Logger::info("[BLE Pair] Authentication OK");
        else Logger::warn("[BLE Pair] Authentication failed");
    }
    uint32_t onPassKeyRequest() override { return 123456; }
    void onPassKeyNotify(uint32_t pass_key) override {}
    bool onConfirmPIN(uint32_t pass_key) override { return true; }
};

"""
content = content.replace(old_class, '')

# Remove security setup in startPairing
old_sec = """// 配置 Just Works 配对（无需 PIN）
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());
    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    """
content = content.replace(old_sec, '')

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("All security code removed - direct connection without PIN")
