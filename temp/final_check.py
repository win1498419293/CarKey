# Final verification
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    header = f.read()

# Check structure
print("=== EmbeddedIndexPage.h ===")
print(f"Size: {len(header)} bytes")
print(f"Has PROGMEM: {'PROGMEM' in header}")
print(f"Has raw string: {'R\"HTMLEND(' in header}")
print(f"Has closing delimiter: {')HTMLEND\"' in header}")

# Verify no non-ASCII
non_ascii = sum(1 for c in header if ord(c) > 127)
print(f"Non-ASCII: {non_ascii}")

# Check HTML content is intact
has_doctype = "<!DOCTYPE html>" in header
has_closing = "</html>" in header
has_entities = "&#" in header
print(f"DOCTYPE: {has_doctype}, Closing: {has_closing}, Entities: {has_entities}")

# Check WebManager.cpp sends
with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    wm = f.read()

import re
sends = re.findall(r'server\.send.*?kEmbeddedIndexPage\)', wm)
print(f"\n=== WebManager.cpp send calls: {len(sends)} ===")
for s in sends:
    print(f"  {s[:70]}...")