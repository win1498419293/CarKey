with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    cpp = f.read()

# Find all return statements in saveSystemConfig
import re
idx_start = cpp.find("bool saveSystemConfig()")
idx_end = cpp.find("\nbool nfcUnlockEnabled", idx_start)
func = cpp[idx_start:idx_end]

# Find bare return;
lines = func.split("\n")
for i, line in enumerate(lines):
    if line.strip() == "return;" or (line.strip().startswith("return;") and "false" not in line and "ok" not in line):
        print(f"Line {i}: {line.strip()}")