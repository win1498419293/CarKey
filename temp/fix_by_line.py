import os

# Process all files: replace lines with non-ASCII chars
# Map of {filename: {line_number: replacement_line}}
# Line numbers are 1-based

fixes = {}

# ===== WebManager.cpp =====
wm = r"D:\CarKey_V5\src\WebManager.cpp"
fixes[wm] = {
    38: 'bool g_wasUnlocked = false;  // Track previous unlock state for change detection',
    41: 'constexpr unsigned long kWifiMaxConnectedMs = 120000;  // 60s cycle limit, yield RF to NFC after',
    154: '    if (now - lastAttemptMs < 10000) return;  // 10 second cooldown between reconnect attempts',
    
    # SPIFFS comment blocks
    185: '// --- SPIFFS / gzip static file serving ---',
    
    # WiFi + NFC comments
    293: '    // WiFi and NFC coordination: initialize RF and PN532 before WiFi',
    297: '    // WiFi begin - initialize TCP/IP stack first',
    303: '    return;',
    304: '    // --- Dead code below (SPIFFS mount bypassed) ---',
    
    # setupRoutes comments
    344: '    // Root route - serve embedded page as fallback when SPIFFS unavailable',
    355: '    // Start engine endpoint',
    389: '    // Stop engine endpoint',
    
    # Post-stop NFC comment  
    621: '    // After WiFi, yield to NFC',
    
    # Route comments
    675: '    // OTA rollback endpoint',
    
    # handle() comments
    677: '    // WiFi connected - block NFC when WiFi is active',
    682: '    // RFManager and NFC coordination: yield during WiFi connection',
    688: '    // Initial WiFi connection: coordinate with RFManager/NFC',
}

# ===== WebManager.h =====
wmh = r"D:\CarKey_V5\include\WebManager.h"
fixes[wmh] = {
    13: 'extern WebManager webManager; // Global instance',
}

# ===== RelayManager.cpp =====
rm = r"D:\CarKey_V5\src\RelayManager.cpp"
fixes[rm] = {
    1: '// RelayManager.cpp - Relay control for engine start/stop/horn/lock',
    # We'll handle RelayManager more carefully since it has more garbled text
}

# Apply fixes
for filepath, line_fixes in fixes.items():
    if not os.path.exists(filepath):
        print(f"  SKIP: {filepath} not found")
        continue
    
    with open(filepath, "rb") as f:
        raw = f.read()
    if raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]
    
    text = raw.decode("utf-8", errors="replace")
    lines = text.split("\n")
    changed = False
    
    for ln, replacement in line_fixes.items():
        idx = ln - 1  # convert to 0-based
        if idx < len(lines):
            old_line = lines[idx]
            # Only replace if it actually has non-ASCII (garbled)
            has_garbled = any(ord(c) > 127 for c in old_line)
            if has_garbled or replacement.startswith('// ---'):
                lines[idx] = replacement
                changed = True
                print(f"  L{ln}: {replacement[:70]}...")
    
    if changed:
        result = "\n".join(lines)
        with open(filepath, "w", encoding="utf-8", newline="") as f:
            f.write(result)
        print(f"  Saved: {os.path.basename(filepath)}")
    else:
        print(f"  No changes: {os.path.basename(filepath)}")