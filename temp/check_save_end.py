with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    cpp = f.read()

# Find the end of saveSystemConfig
idx = cpp.find("Logger::info(\"[Config] Settings saved")
if idx > 0:
    print(cpp[idx:idx+200])
else:
    print("Settings saved log not found")