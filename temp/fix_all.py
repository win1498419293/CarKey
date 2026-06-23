import os

# Comprehensive fix: replace garbled lines with English
# Map: {filepath: {line_number: replacement_line}}

FIXES = {}

base = r"D:\CarKey_V5"

# ===== WebManager.cpp =====
FIXES[base + r"\src\WebManager.cpp"] = {
    39: 'bool g_wasUnlocked = false;  // Track previous unlock state for auth change detection',
    155: '    if (now - lastAttemptMs < 10000) return;  // 10 second cooldown between reconnect attempts',
    186: '// --- SPIFFS / gzip static file fallback serving ---',
    211: '    // Check if client supports gzip encoding',
    220: '    // Serve gzip compressed file if available',
    234: '    // Serve plain file from SPIFFS',
    244: '    // SPIFFS file not found, serve embedded fallback',
    290: '    // WiFi and NFC coordination: init RF and PN532 before WiFi connection',
    302: '    WiFi.mode(WIFI_STA);  // Initialize TCP/IP stack, then WiFi',
    308: '    // Fall through: SPIFFS mount and WiFi connection',
    350: '    // Root route - serve embedded page as SPIFFS fallback',
    523: '    // WiFi SSID / password update - restart if changed',
    629: '    // After WiFi connected, yield airtime to NFC',
    631: '    // BLE pairing API endpoints',
    685: '        // WiFi connected: block config access when WiFi is active',
    687: '            // RFManager and NFC query coordination when WiFi is connected',
    696: '    // Initial WiFi connection phase: coordinate with RFManager NFC queries',
}

# ===== RelayManager.cpp =====
FIXES[base + r"\src\RelayManager.cpp"] = {
    16: '    pinMode(PIN_HORN, OUTPUT); // Horn pin output',
    18: '    // Engine control pins (all active LOW, relay module active HIGH)',
    21: '    digitalWrite(PIN_HORN, LOW); // Horn off',
    25: '// Horn control',
    31: '    digitalWrite(PIN_HORN, HIGH); // Horn relay on',
    35: '    digitalWrite(PIN_HORN, LOW); // Horn off',
    57: '        Logger::info("[RelayManager] Start charge verify: voltage (" + String(BATTERY_ALTERNATOR_MIN_V, 1)',
    58: '                     + "V) check in progress");',
    63: '        Logger::error("[RelayManager] Start timeout: " + String(START_CHARGE_VERIFY_TIMEOUT_MS / 1000)',
    64: '                      + " seconds, alternator voltage not reached");',
    72: '        Logger::info("[RelayManager] Charge verify: alternator voltage OK");',
    77: '        Logger::error("[RelayManager] Low battery (" + String(batteryVoltage.getVoltage(), 2) + "V < "',
    78: '                      + String(BATTERY_LOW_START_BLOCK_V, 1) + "V), start blocked");',
    83: '    // Start engine sequence',
    89: '        Logger::error("[RelayManager] Start failed: engine is already running");',
    94: '    // Begin start sequence',
    96: '    Logger::info("[RelayManager] Engine start sequence started...");',
    98: '    // 1. IGN ON',
    100: '    Logger::info(" -> 1. IGN power ON...");',
    101: '    delay(3000); // Wait 2-3 seconds for fuel pump to prime',
    103: '    // 2. Cranking',
    104: '    Logger::info(" -> 2. Cranking starter motor!");',
    107: '    // Crank time: typically 0.8 to 1.2 seconds',
    110: '    // 3. Release starter',
    112: '    Logger::info(" -> 3. Starter released, engine running");',
    116: '    Logger::info("[RelayManager] Charge verify started (" + String(START_CHARGE_VERIFY_TIMEOUT_MS / 1000)',
    117: '                 + " second timeout, threshold " + String(BATTERY_ALTERNATOR_MIN_V, 1) + "V)");',
    122: '    Logger::info("[RelayManager] Engine stop...");',
    123: '    digitalWrite(PIN_START, LOW); // Starter relay OFF',
    124: '    digitalWrite(PIN_IGN, LOW);   // IGN power OFF',
}

# ===== NFCManager.cpp =====
FIXES[base + r"\src\NFCManager.cpp"] = {
    10: '    Logger::info("[NFC] Initializing PN532...");',
    20: '            Logger::error("PN532 not found (attempt " + String(attempt) + "/3)");',
    27: '            Logger::error("PN532 SAMConfig failed (attempt " + String(attempt) + "/3)");',
    33: '        Logger::info("PN532 init success (attempt " + String(attempt) + "/3)");',
}

# ===== Config.cpp ===== (fix bt_name garbled char)
FIXES[base + r"\src\Config.cpp"] = {
    11: 'String bt_name = "REDMI Turbo 4 Pro";',
    29: '    bt_name = prefs.getString("bt_name", "REDMI Turbo 4 Pro");',
    34: '    if (bt_name.length() == 0) bt_name = "REDMI Turbo 4 Pro";',
    93: '    {"REDMI Turbo 4 Pro", "", -85},',
}

# ===== BLEManager.cpp =====
FIXES[base + r"\src\BLEManager.cpp"] = {
    223: '// ================== Public API ==================',
    262: '    // Start BLE scan - configure device name and scan params',
}

# ===== BLEManager.h =====
FIXES[base + r"\include\BLEManager.h"] = {
    33: '    // Public state accessors',
    64: '    // Private helpers (char arrays, not String)',
}

# ===== BatteryVoltage.h =====
FIXES[base + r"\include\BatteryVoltage.h"] = {
    4: '// Battery voltage ADC monitoring + low voltage alert (< 11.8V block remote start)',
}

# ===== Logger.h =====
FIXES[base + r"\include\Logger.h"] = {
    4: '// Unified logging interface with levels: INFO / WARN / ERROR / DEBUG',
}

# ===== RelayManager.h =====
FIXES[base + r"\include\RelayManager.h"] = {
    13: '    // Horn control',
    20: '    bool engineRunning = false; // Engine running state',
    21: '    unsigned long startChargeVerifyBeginMs = 0; // 0 = not verifying',
}

# ===== WebManager.h =====
FIXES[base + r"\include\WebManager.h"] = {
    18: 'extern WebManager webManager; // Global singleton instance',
}

# ===== AuthManager.cpp =====
FIXES[base + r"\src\AuthManager.cpp"] = {
    8: '// NFC auth: see Config.cpp authorizedCards list',
    19: '        return false;  // Need at least 4 byte UID',
}

# ===== TaskManager.cpp =====
FIXES[base + r"\src\TaskManager.cpp"] = {
    189: '        // NFC scan burst: suppress BLE and WiFi during active NFC scan',
    265: '            bleManager.stopActiveScan();  // NFC active: pause BLE scan',
}

# ===== Single-char BOM fixes for headers =====
for f in [r"\include\AuthManager.h", r"\include\Config.h", r"\include\OTAManager.h", r"\include\RFManager.h"]:
    FIXES[base + f] = {}  # Will just strip non-ASCII

for f in [r"\src\main.cpp", r"\src\OTAManager.cpp", r"\src\RFManager.cpp", r"\src\StateMachine.cpp"]:
    FIXES[base + f] = {}

# Apply all fixes
total_fixed = 0
for filepath, line_fixes in FIXES.items():
    if not os.path.exists(filepath):
        continue
    
    with open(filepath, "rb") as f:
        raw = f.read()
    if raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]
    
    text = raw.decode("utf-8", errors="replace")
    lines = text.split("\n")
    changed = False
    
    for ln, replacement in line_fixes.items():
        idx = ln - 1
        if idx < len(lines):
            lines[idx] = replacement
            changed = True
            total_fixed += 1
    
    # Also strip any remaining non-ASCII from non-fixed lines
    # (clean up stray BOM/corrupted single chars)
    for i, line in enumerate(lines):
        if any(ord(c) > 127 for c in line):
            # Replace non-ASCII chars with '?'
            clean = ''.join(c if ord(c) < 128 else '?' for c in line)
            # But don't mark as changed if it's already been fixed above
            old_clean = ''.join(c if ord(c) < 128 else '?' for c in lines[i])
            if clean != old_clean:
                continue  # already replaced
    
    if changed:
        result = "\n".join(lines)
        with open(filepath, "w", encoding="utf-8", newline="") as f:
            f.write(result)

print(f"Fixed {total_fixed} lines across all files")