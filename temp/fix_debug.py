with open(r"D:\CarKey_V5\src\BLEManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Fix the isPairedDevice function - replace the whole thing
old = """bool BLEManager::isPairedDevice(const char* mac, const char* name) const {
    if (!devicePaired) return false;
    String nm = normalizeMac(String(pairedDeviceMac)), dm = normalizeMac(String(mac));
    if (nm.length()>0 && dm==nm) { Logger::info("[BLE-DBG] MAC match: " + String(pairedDeviceMac) + " == " + dm); return true; }
    String pn = String(pairedDeviceName); pn.toUpperCase();
    String dn = String(name ? name : ""); dn.toUpperCase();
    if (pn.length()>0 && dn.length()>0 && dn==pn) { Logger::info("[BLE-DBG] Name match: " + pn + " == " + dn); return true; }
    return false;"""

new = """bool BLEManager::isPairedDevice(const char* mac, const char* name) const {
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

if old in content:
    content = content.replace(old, new)
else:
    print("Old pattern not found, searching...")
    idx = content.find("bool BLEManager::isPairedDevice")
    if idx >= 0:
        # Find the closing brace
        brace_count = 0
        end = idx
        for i in range(idx, len(content)):
            if content[i] == '{': brace_count += 1
            elif content[i] == '}':
                brace_count -= 1
                if brace_count == 0:
                    end = i + 1
                    break
        print(f"Found function at {idx}-{end}")
        content = content[:idx] + new + content[end:]

with open(r"D:\CarKey_V5\src\BLEManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("Fixed")
