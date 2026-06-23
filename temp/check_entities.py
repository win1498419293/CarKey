with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

# Find the first entity
idx = content.find("&#")
if idx >= 0:
    print(f"First entity at position {idx}:")
    print(repr(content[idx:idx+40]))
else:
    print("No &# entities found!")

# Check for double-escaping
if "&amp;#" in content:
    print("WARNING: Double-escaped entities found!")
else:
    print("No double-escaping detected")

# Find a few more samples
import re
entities = re.findall(r'&#\d+;', content)
print(f"\nTotal entities: {len(entities)}")
print(f"First 5: {entities[:5]}")