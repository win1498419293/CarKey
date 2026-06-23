with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Use the macro instead of hardcoded 40
content = content.replace("nfcManager.pollCard(uid, uidLength, 40)", "nfcManager.pollCard(uid, uidLength, NFC_SCAN_POLL_TIMEOUT_MS)")

with open(r"D:\CarKey_V5\src\TaskManager.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print("Fixed: using NFC_SCAN_POLL_TIMEOUT_MS macro (50ms)")