with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Check current state of the three send calls
import re
sends = re.findall(r'server\.send.*?kEmbeddedIndexPage\)', content)
for i, s in enumerate(sends):
    print(f"Send {i+1}: {s[:80]}...")