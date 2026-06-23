with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

print("First 5 lines:")
for line in content.split("\n")[:5]:
    print(f"  {line[:100]}")

print("\nLast 3 lines:")
for line in content.split("\n")[-3:]:
    print(f"  {line[:100]}")

# Check for issues
if '\\"' in content:
    # C escape for double-quote - this is normal in PROGMEM strings
    print("\nHas escaped quotes: YES (normal)")
if '\\\\' in content:
    print("Has double-backslash: YES (may be issue if too many)")
    
# Verify the HTML entities survived
if "&#25377;" in content:
    print("Entity &#25377; present: YES")
if "&#21551;&#21160;" in content:
    print("Entity &#21551;&#21160; (start) present: YES")