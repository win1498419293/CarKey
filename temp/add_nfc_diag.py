with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    tm = f.read()

# Add logging when NFC task starts and when authMethodNFC changes
# Find: if (!authMethodNFC) {
# Add static tracking of previous state
old_nfc_check = """        if (!authMethodNFC) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }"""

new_nfc_check = """        static bool lastNfcState = true;
        if (!authMethodNFC) {
            if (lastNfcState) {
                Logger::info("[NFC Task] NFC disabled, pausing scan");
                lastNfcState = false;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (!lastNfcState) {
            Logger::info("[NFC Task] NFC enabled, resuming scan");
            lastNfcState = true;
        }"""

if old_nfc_check in tm:
    tm = tm.replace(old_nfc_check, new_nfc_check)
    print("Added NFC task state change logging")
else:
    print("NFC check pattern not found")

# Also add a periodic heartbeat in NFC task
old_heartbeat = """        if (millis() - lastHeartbeat > 30000) {
            lastHeartbeat = millis();
        }"""

new_heartbeat = """        if (millis() - lastHeartbeat > 30000) {
            lastHeartbeat = millis();
            Logger::info("[NFC Task] Heartbeat - authMethodNFC=" + String(authMethodNFC ? "ON" : "OFF"));
        }"""

if old_heartbeat in tm:
    tm = tm.replace(old_heartbeat, new_heartbeat)
    print("Added NFC task heartbeat logging")
else:
    print("Heartbeat pattern not found")

with open(r"D:\CarKey_V5\src\TaskManager.cpp", "w", encoding="utf-8") as f:
    f.write(tm)