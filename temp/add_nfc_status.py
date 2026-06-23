with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Find the status JSON - add nfc_scan field
# Look for "ble_scan" in the status response
old = 'json += "\\\"ble_scan\\\":" + String(bleScanEnabled ? "true" : "false") + ",";'
new = 'json += "\\\"ble_scan\\\":" + String(bleScanEnabled ? "true" : "false") + ",";\n        json += "\\\"nfc_scan\\\":" + String(authMethodNFC ? "true" : "false") + ",";'

if old in content:
    content = content.replace(old, new)
    print("Added nfc_scan to /api/status")
else:
    print("WARNING: ble_scan field not found in status response")

with open(r"D:\CarKey_V5\src\WebManager.cpp", "w", encoding="utf-8") as f:
    f.write(content)