with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Change saveSystemConfig to return bool
# 1. Fix function signature
old_sig = "void saveSystemConfig() {"
new_sig = "bool saveSystemConfig() {"
content = content.replace(old_sig, new_sig)

# 2. Fix early return when begin fails
old_fail = 'Logger::error("[Config] Failed to open NVS namespace carkey_sys for write");\n        return;'
new_fail = 'Logger::error("[Config] Failed to open NVS namespace carkey_sys for write");\n        return false;'
content = content.replace(old_fail, new_fail)

# 3. Fix normal return at end - add return ok
old_end_save = 'Logger::info("[Config] Settings saved to NVS (nfc=\"" + String(authMethodNFC ? \"ON\" : \"OFF\") + \")\");'
new_end_save = 'Logger::info("[Config] Settings saved to NVS (nfc=\"" + String(authMethodNFC ? \"ON\" : \"OFF\") + \")\");\n    return ok;'
content = content.replace(old_end_save, new_end_save)

with open(r"D:\CarKey_V5\src\Config.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print("saveSystemConfig now returns bool")

# Also update Config.h
with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    hdr = f.read()

hdr = hdr.replace("void saveSystemConfig();", "bool saveSystemConfig();")
with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(hdr)

print("Updated Config.h declaration")

# Now update WebManager.cpp to check the return value
with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    wm = f.read()

# Find saveSystemConfig() call in update_config handler
old_call = "saveSystemConfig();\n        Logger::info(\"[System] Config saved to NVS\");"
new_call = """bool saved = saveSystemConfig();
        if (!saved) {
            Logger::error("[System] Config save FAILED!");
            sendNoCacheHeaders();
            server.send(500, "text/plain", "Save Failed: NVS error");
            return;
        }
        Logger::info("[System] Config saved to NVS");"""

if old_call in wm:
    wm = wm.replace(old_call, new_call)
    print("Added save result check to WebManager")
else:
    print("WARNING: saveSystemConfig call not found in expected format")
    # Try to find it
    idx = wm.find("saveSystemConfig()")
    if idx > 0:
        print(f"  Found at {idx}: ...{wm[idx-20:idx+50]}...")

with open(r"D:\CarKey_V5\src\WebManager.cpp", "w", encoding="utf-8") as f:
    f.write(wm)

print("\nDone. If NVS write fails, API now returns 500 instead of 200 (silent failure).")