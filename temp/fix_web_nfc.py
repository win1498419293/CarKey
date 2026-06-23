with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    wm = f.read()

# Remove nfcManager.init() from the web handler - just set the flag
# The NFC task will pick up the change on next loop iteration
old = """        if (server.hasArg("nfc_scan")) {
            authMethodNFC = (server.arg("nfc_scan") == "true");
            if (!authMethodNFC) {
                Logger::info("[Config] NFC scan disabled");
            } else {
                Logger::info("[Config] NFC scan enabled");
                nfcManager.init();
            }
        }"""

new = """        if (server.hasArg("nfc_scan")) {
            authMethodNFC = (server.arg("nfc_scan") == "true");
            Logger::info(String("[Config] NFC scan ") + (authMethodNFC ? "ENABLED" : "DISABLED"));
        }"""

if old in wm:
    wm = wm.replace(old, new)
    print("Removed nfcManager.init() from web handler")
else:
    print("Pattern not found, checking...")
    idx = wm.find('server.hasArg("nfc_scan")')
    if idx > 0:
        print(wm[idx:idx+350])

with open(r"D:\CarKey_V5\src\WebManager.cpp", "w", encoding="utf-8") as f:
    f.write(wm)

print("\nNow let's add NFC task diagnostic logging:")