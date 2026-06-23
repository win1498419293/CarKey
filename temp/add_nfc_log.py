with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Add logging to loadSystemConfig for auth_nfc
old_load = "authMethodNFC = prefs.getBool(\"auth_nfc\", true);"
new_load = "authMethodNFC = prefs.getBool(\"auth_nfc\", true);\n    Logger::info(\"[Config] Loaded auth_nfc: \" + String(authMethodNFC ? \"true\" : \"false\"));"
content = content.replace(old_load, new_load)

# Add logging to saveSystemConfig before saving auth_nfc
old_save = "ok = ok && prefs.putBool(\"auth_nfc\", authMethodNFC);"
new_save = "Logger::info(\"[Config] Saving auth_nfc: \" + String(authMethodNFC ? \"true\" : \"false\"));\n    ok = ok && prefs.putBool(\"auth_nfc\", authMethodNFC);"
content = content.replace(old_save, new_save)

# Also log after saveSystemConfig completes
old_end = "Logger::info(\"[Config] Settings saved to NVS\");"
new_end = "Logger::info(\"[Config] Settings saved to NVS (nfc=\" + String(authMethodNFC ? \"ON\" : \"OFF\") + \")\");"
content = content.replace(old_end, new_end)

with open(r"D:\CarKey_V5\src\Config.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print("Added NFC logging to Config.cpp")

# Verify
for check in ["Loaded auth_nfc", "Saving auth_nfc", "nfc="]:
    print(f"  {'[OK]' if check in content else '[MISSING]'} {check}")