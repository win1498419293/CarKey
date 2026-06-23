with open(r"D:\CarKey_V5\src\BLEManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# 1. In scanner callback, log all detected device names for debugging
old_cb = """void onResult(BLEAdvertisedDevice advertisedDevice) override {
        static unsigned long lastLogMs=0; auto now=millis(); if(now-lastLogMs>2000){lastLogMs=now;}
        String deviceMac = normalizeMac(advertisedDevice.getAddress().toString().c_str());
        String deviceName = advertisedDevice.getName().c_str(); deviceName.toUpperCase();
        const int rssi = advertisedDevice.getRSSI();
        if (bleManager.isPairedDevice(deviceMac.c_str(), deviceName.c_str()) && rssi >= -85) {"""

new_cb = """void onResult(BLEAdvertisedDevice advertisedDevice) override {
        static unsigned long lastLogMs=0; auto now=millis(); if(now-lastLogMs>3000){lastLogMs=now;}
        String deviceMac = normalizeMac(advertisedDevice.getAddress().toString().c_str());
        String deviceName = advertisedDevice.getName().c_str(); deviceName.toUpperCase();
        const int rssi = advertisedDevice.getRSSI();
        // 调试：记录所有有名称的设备
        if (deviceName.length() > 0) {
            Logger::info("[BLE-SCAN] " + deviceName + " " + deviceMac + " RSSI:" + String(rssi));
        }
        if (bleManager.isPairedDevice(deviceMac.c_str(), deviceName.c_str()) && rssi >= -85) {"""

content = content.replace(old_cb, new_cb, 1)

# 2. Rewrite savePairedDevice to also capture the pairing name from PairServerCallbacks
# The PairServerCallbacks::onConnect currently only passes MAC. Need to also pass a name.
# But we don't have the phone's BLE name during connection. 
# Instead, use the stored bt_name as a hint, or just store a generic name.

# 3. Rewrite isPairedDevice to prefer name matching, and accept partial MAC match
old_is = """bool BLEManager::isPairedDevice(const char* mac, const char* name) const {
    if (!devicePaired) return false;
    String nm = normalizeMac(String(pairedDeviceMac));
    String dm = normalizeMac(String(mac));
    if (nm.length()>0 && dm==nm) {
        Logger::info("[BLE-DBG] MAC match! " + String(pairedDeviceMac));
        return true;
    }
    String pn = String(pairedDeviceName); pn.toUpperCase();
    String dn = String(name ? name : ""); dn.toUpperCase();
    if (pn.length()>0 && dn.length()>0 && dn==pn) {
        Logger::info("[BLE-DBG] Name match! " + pn);
        return true;
    }
    return false;
}"""

new_is = """bool BLEManager::isPairedDevice(const char* mac, const char* name) const {
    if (!devicePaired) return false;
    // 优先用名称匹配（手机随机MAC会变）
    String pn = String(pairedDeviceName); pn.toUpperCase();
    String dn = String(name ? name : ""); dn.toUpperCase();
    if (pn.length()>0 && dn.length()>0 && dn.indexOf(pn)>=0) {
        Logger::info("[BLE-DBG] Name match: " + dn + " contains " + pn);
        return true;
    }
    // 其次用MAC匹配
    String nm = normalizeMac(String(pairedDeviceMac));
    String dm = normalizeMac(String(mac));
    if (nm.length()>0 && dm==nm) {
        Logger::info("[BLE-DBG] MAC match: " + String(pairedDeviceMac));
        return true;
    }
    return false;
}"""

content = content.replace(old_is, new_is)

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("Name-based matching implemented")
