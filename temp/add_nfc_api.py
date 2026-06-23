with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Find the ble_scan handling block and add nfc_scan after it
old_ble_block = '''        if (server.hasArg("ble_scan")) {
            bleScanEnabled = (server.arg("ble_scan") == "true");
            authMethodBLE = bleScanEnabled;
            if (!bleScanEnabled) {
                bleManager.stopActiveScan();
            } else {
                bleManager.init();
            }
        }'''

new_block = '''        if (server.hasArg("ble_scan")) {
            bleScanEnabled = (server.arg("ble_scan") == "true");
            authMethodBLE = bleScanEnabled;
            if (!bleScanEnabled) {
                bleManager.stopActiveScan();
            } else {
                bleManager.init();
            }
        }
        if (server.hasArg("nfc_scan")) {
            authMethodNFC = (server.arg("nfc_scan") == "true");
            if (!authMethodNFC) {
                Logger::info("[Config] NFC scan disabled");
            } else {
                Logger::info("[Config] NFC scan enabled");
                nfcManager.init();
            }
        }'''

if old_ble_block in content:
    content = content.replace(old_ble_block, new_block)
    print("Added nfc_scan to update_config")
else:
    print("WARNING: ble_scan block not found!")

with open(r"D:\CarKey_V5\src\WebManager.cpp", "w", encoding="utf-8") as f:
    f.write(content)
print("WebManager.cpp updated")