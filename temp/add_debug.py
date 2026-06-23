with open(r"D:\CarKey_V5\src\BLEManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Add logging in the scanner callback
old = "bleManager.onAuthorizedHit(millis(), bleManager.getPairedDeviceName());"
new = """Logger::info("[BLE-DBG] Paired device detected: " + deviceMac + " " + deviceName);
                bleManager.onAuthorizedHit(millis(), bleManager.getPairedDeviceName());"""

content = content.replace(old, new, 1)

# Add logging in isPairedDevice to see comparison
old2 = "bool BLEManager::isPairedDevice(const char* mac, const char* name) const {"
new2 = """bool BLEManager::isPairedDevice(const char* mac, const char* name) const {
    if (!devicePaired) return false;
    String nm = normalizeMac(String(pairedDeviceMac)), dm = normalizeMac(String(mac));
    if (nm.length()>0 && dm==nm) { Logger::info("[BLE-DBG] MAC match: " + String(pairedDeviceMac) + " == " + dm); return true; }
    String pn = String(pairedDeviceName); pn.toUpperCase();
    String dn = String(name ? name : ""); dn.toUpperCase();
    if (pn.length()>0 && dn.length()>0 && dn==pn) { Logger::info("[BLE-DBG] Name match: " + pn + " == " + dn); return true; }
    return false;"""

old3 = """bool BLEManager::isPairedDevice(const char* mac, const char* name) const {
    if (!devicePaired) return false;
    String nm = normalizeMac(String(pairedDeviceMac)), dm = normalizeMac(String(mac));
    if (nm.length()>0 && dm==nm) return true;
    String pn = String(pairedDeviceName), dn = String(name ? name : "");
    pn.toUpperCase(); dn.toUpperCase();
    return pn.length()>0 && dn.length()>0 && dn==pn;
}"""

content = content.replace(old3, new2)

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("Debug logging added")
