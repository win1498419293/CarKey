import re

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Add includes
content = content.replace(
    '#include <BLEServer.h>',
    '#include <BLEServer.h>\n#include <BLESecurity.h>'
)

# Add security callbacks class before PairServerCallbacks
old_pair_class = "class PairServerCallbacks"
security_class = """// BLE 安全回调 - Just Works 配对（无需 PIN）
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

content = content.replace(old_pair_class, security_class + old_pair_class)

# Add security configuration in startPairing after pServer->setCallbacks
old_setup = """pServer->setCallbacks(new PairServerCallbacks());
    pService = pServer->createService(kPairServiceUUID);"""

new_setup = """pServer->setCallbacks(new PairServerCallbacks());
    // 配置 Just Works 配对（无需 PIN）
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());
    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pService = pServer->createService(kPairServiceUUID);"""

content = content.replace(old_setup, new_setup)

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("Security callbacks added")
