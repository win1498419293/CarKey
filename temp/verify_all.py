# Verify Config.cpp
with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    cpp = f.read()
for check in ["bool saveSystemConfig()", "return false;", "return ok;"]:
    print(f"  Config.cpp: {'[OK]' if check in cpp else '[MISSING]'} {check}")

# Verify Config.h
with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    hdr = f.read()
print(f"  Config.h: {'[OK]' if 'bool saveSystemConfig();' in hdr else '[MISSING]'} bool decl")

# Verify WebManager.cpp
with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    wm = f.read()
for check in ["bool saved = saveSystemConfig()", "server.send(500", "Save Failed"]:
    print(f"  WebManager: {'[OK]' if check in wm else '[MISSING]'} {check}")