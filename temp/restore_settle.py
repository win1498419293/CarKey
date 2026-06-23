with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    tm = f.read()

old = r"""RFManager::beginNFC();
            nfcManager.reconfig();

            uint8_t uid[7]"""

new = r"""RFManager::beginNFC();
            nfcManager.reconfig();
            delay(NFC_SCAN_SETTLE_MS);

            uint8_t uid[7]"""

tm = tm.replace(old, new)
with open(r"D:\CarKey_V5\src\TaskManager.cpp", "w", encoding="utf-8") as f:
    f.write(tm)
print("Restored settle delay")

# Verify
idx = tm.find("g_nfcScanning = true;")
if idx > 0:
    section = tm[idx:idx+250]
    if "reconfig()" in section and "settle" in section.lower():
        print("Scan code correct!")
    else:
        print("Scan code: " + section[:200])