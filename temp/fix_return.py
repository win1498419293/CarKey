with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    cpp = f.read()

# Add return ok before the closing brace of saveSystemConfig
old = 'Logger::info("[Config] Settings saved to NVS (nfc=" + String(authMethodNFC ? "ON" : "OFF") + ")");\n\n}'
new = 'Logger::info("[Config] Settings saved to NVS (nfc=" + String(authMethodNFC ? "ON" : "OFF") + ")");\n    return ok;\n}'

if old in cpp:
    cpp = cpp.replace(old, new)
    print("Added return ok")
else:
    print("Pattern not found")

with open(r"D:\CarKey_V5\src\Config.cpp", "w", encoding="utf-8") as f:
    f.write(cpp)

print("return ok: " + ("YES" if "return ok;" in cpp else "NO"))