with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    cpp = f.read()

old = """    if (!ok) {
        Logger::error("[Config] Failed to persist one or more settings to NVS");
        return;
    }"""

new = """    if (!ok) {
        Logger::error("[Config] Failed to persist one or more settings to NVS");
        return false;
    }"""

cpp = cpp.replace(old, new)

with open(r"D:\CarKey_V5\src\Config.cpp", "w", encoding="utf-8") as f:
    f.write(cpp)

print("Fixed bare return -> return false")
print("Verified: " + ("return false;" in cpp and "return;\n" not in cpp[cpp.find("bool saveSystemConfig"):]))