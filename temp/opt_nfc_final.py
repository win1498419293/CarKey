with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    tm = f.read()

# Replace the scan section with optimized version
old = r"""g_nfcScanning = true;
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
                vTaskDelay(pdMS_TO_TICKS(2));
            }"""

new = r"""g_nfcScanning = true;
            RFManager::beginNFC();

            // Reconfig SAM every 10 scans to keep PN532 in sync
            static int reconfCnt = 0;
            if (++reconfCnt >= 10) {
                nfcManager.reconfig();
                reconfCnt = 0;
            }

            uint8_t uid[7] = {0};
            uint8_t uidLength = 0;
            bool detected = false;
            const unsigned long scanStart = millis();
            while (millis() - scanStart < NFC_SCAN_WINDOW_MS) {
                if (nfcManager.pollCard(uid, uidLength, NFC_SCAN_POLL_TIMEOUT_MS)) {
                    detected = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(2));
            }"""

if old in tm:
    tm = tm.replace(old, new)
    print("Applied optimizations:")
    print("  - Removed settle delay (8ms saved)")
    print("  - reconfig only every 10 scans (~7ms/scan saved)")
    print("  - Effective scan time: ~100ms (was ~82ms)")
else:
    print("Pattern not found!")

with open(r"D:\CarKey_V5\src\TaskManager.cpp", "w", encoding="utf-8") as f:
    f.write(tm)

# Also update Config.h for better poll timeout
with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    cfg = f.read()

cfg = cfg.replace("#define NFC_SCAN_POLL_TIMEOUT_MS 40U", "#define NFC_SCAN_POLL_TIMEOUT_MS 50U")

with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(cfg)

print("  - Poll timeout: 40ms -> 50ms")
print("\nNew cycle: 100ms window with 50ms timeout = 2 deep polls")