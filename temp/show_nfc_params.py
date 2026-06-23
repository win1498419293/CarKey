with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    cfg = f.read()

# Show NFC parameters
import re
for m in re.finditer(r'#define NFC_\w+.*', cfg):
    print(m.group())