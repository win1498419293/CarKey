with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Fix: remove reconfig from scan loop, increase poll timeout, reduce overhead
old_scan = """        // NFC scan burst: suppress BLE and WiFi during active NFC scan
        if (millis() - lastScanMs >= NFC_SCAN_INTERVAL_MS) {
            lastScanMs = millis();
            g_nfcScanning = true;
            RFManager::beginNFC();
            nfcManager.reconfig();
            delay(NFC_SCAN_SETTLE_MS);

            uint8_t uid[7] = {0};
            uint8_t uidLength = 0;
            bool detected = false;
            const unsigned long scanStart = millis();
            while (millis() - scanStart < NFC_SCAN_WINDOW_MS) {
                if (nfcManager.pollCard(uid, uidLength, NFC_SCAN_POLL_TIMEOUT_MS)) {
                    detected = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(NFC_SCAN_RETRY_DELAY_MS));
            }"""

new_scan = """        // NFC scan burst: reduce WiFi TX power during poll
        if (millis() - lastScanMs >= NFC_SCAN_INTERVAL_MS) {
            lastScanMs = millis();
            g_nfcScanning = true;
            RFManager::beginNFC();

            uint8_t uid[7] = {0};
            uint8_t uidLength = 0;
            bool detected = false;
            const unsigned long scanStart = millis();
            while (millis() - scanStart < NFC_SCAN_WINDOW_MS) {
                if (nfcManager.pollCard(uid, uidLength, 40)) {
                    detected = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(2));
            }"""

if old_scan in content:
    content = content.replace(old_scan, new_scan)
    print("Optimized NFC scan section")
else:
    print("NOT FOUND - checking...")
    idx = content.find("NFC scan burst")
    if idx > 0:
        print(f"Found at {idx}: {content[idx:idx+100]}")

# Also increase NFC_SCAN_WINDOW_MS and decrease interval in Config.h
with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    config = f.read()

# Make scan window longer (gives card more time)
config = config.replace("#define NFC_SCAN_WINDOW_MS 80U", "#define NFC_SCAN_WINDOW_MS 100U")
# Make poll timeout slightly longer
config = config.replace("#define NFC_SCAN_POLL_TIMEOUT_MS 20U", "#define NFC_SCAN_POLL_TIMEOUT_MS 50U")

with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(config)
print("Updated NFC_SCAN_WINDOW_MS to 100, NFC_SCAN_POLL_TIMEOUT_MS to 50")

# Save TaskManager
with open(r"D:\CarKey_V5\src\TaskManager.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print("\nChanges:")
print("  1. Removed nfcManager.reconfig() from scan loop (not needed each cycle)")
print("  2. Removed 8ms settle delay")
print("  3. Poll timeout: 20ms -> 40ms (inline)")
print("  4. Scan window: 80ms -> 100ms (Config.h)")
print("  5. NFC_SCAN_POLL_TIMEOUT_MS: 20 -> 50 (Config.h)")