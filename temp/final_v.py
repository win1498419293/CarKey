import re

# Check EmbeddedIndexPage.h
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    h = f.read()

print("=== EmbeddedIndexPage.h ===")
print(f"Size: {len(h)} bytes, ASCII-only: {sum(1 for c in h if ord(c) > 127) == 0}")
print(f"Has PROGMEM: {'PROGMEM' in h}")
print(f"Has DOCTYPE: {'<!DOCTYPE html>' in h}")
print(f"Has </html>: {'</html>' in h}")
print(f"Entity count: {len(re.findall(r'&#\d+;', h))}")

# Check WebManager.cpp
with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    w = f.read()

sends = re.findall(r'server\.send_P\(200, "text/html; charset=utf-8", kEmbeddedIndexPage\)', w)
print(f"\n=== WebManager.cpp ===")
print(f"send_P with charset=utf-8: {len(sends)} occurrences")

# Check no raw 'text/html' without charset
bare = re.findall(r'"text/html"(?!;)', w)
print(f"Bare text/html (no charset): {len(bare)}")