with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    cpp = f.read()

idx_start = cpp.find("bool saveSystemConfig()")
idx_end = cpp.find("\nbool nfcUnlockEnabled", idx_start)
func = cpp[idx_start:idx_end]
lines = func.split("\n")
for i in range(15, 25):
    if i < len(lines):
        print(f"L{i}: {lines[i]}")