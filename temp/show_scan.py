with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    tm = f.read()

# Show the full NFC scan burst
idx = tm.find("g_nfcScanning = true;")
if idx > 0:
    print(tm[idx:idx+500])