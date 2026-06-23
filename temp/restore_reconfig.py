with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    tm = f.read()

# Add reconfig back (but keep settle delay removed)
old = r"""g_nfcScanning = true;
            RFManager::beginNFC();

            uint8_t uid[7]"""

new = r"""g_nfcScanning = true;
            RFManager::beginNFC();
            nfcManager.reconfig();

            uint8_t uid[7]"""

if old in tm:
    tm = tm.replace(old, new)
    print("Restored nfcManager.reconfig() in scan loop")
else:
    print("Pattern not found, checking NFC scan section...")
    idx = tm.find("g_nfcScanning = true;")
    if idx > 0:
        print(tm[idx:idx+200])

with open(r"D:\CarKey_V5\src\TaskManager.cpp", "w", encoding="utf-8") as f:
    f.write(tm)