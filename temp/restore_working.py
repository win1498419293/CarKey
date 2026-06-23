with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    tm = f.read()

# Replace with: reconfig every scan + settle delay restored
old = r"""g_nfcScanning = true;
            RFManager::beginNFC();

            // Reconfig SAM every 10 scans to keep PN532 in sync
            static int reconfCnt = 0;
            if (++reconfCnt >= 10) {
                nfcManager.reconfig();
                reconfCnt = 0;
            }

            uint8_t uid[7] = {0};"""

new = r"""g_nfcScanning = true;
            RFManager::beginNFC();
            nfcManager.reconfig();
            delay(NFC_SCAN_SETTLE_MS);

            uint8_t uid[7] = {0};"""

if old in tm:
    tm = tm.replace(old, new)
    print("Restored: reconfig every scan + settle delay")
else:
    # Try with different reconfCnt value
    idx = tm.find("reconfCnt")
    if idx > 0:
        print(f"Found reconfCnt at {idx}, checking context...")
        print(tm[idx-50:idx+200])

with open(r"D:\CarKey_V5\src\TaskManager.cpp", "w", encoding="utf-8") as f:
    f.write(tm)

# Verify
idx = tm.find("g_nfcScanning = true;")
if idx > 0:
    section = tm[idx:idx+220]
    has_reconfig = "reconfig()" in section
    has_settle = "SETTLE_MS" in section
    print(f"reconfig: {'YES' if has_reconfig else 'NO'}")
    print(f"settle:   {'YES' if has_settle else 'NO'}")