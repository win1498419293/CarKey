with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

# Simple reconstruction
import re
pattern = re.compile(r'^"(.+?)\\n"$')
reconstructed = []
for line in content.split("\n"):
    m = pattern.match(line)
    if m:
        reconstructed.append(m.group(1))

html = "\n".join(reconstructed)

# Check start and end
print(f"HTML starts with: {html[:50]}")
print(f"HTML ends with: {html[-50:]}")
print(f"Total lines reconstructed: {len(reconstructed)}")

# Verify all key JS functions exist
key_funcs = ["openAuth", "closeAuth", "doStart", "doApi", "openSettings",
             "closeSettings", "saveSettings", "rebootDevice", "updateStatus",
             "updatePairStatus", "startPair", "stopPair", "clearPair", "addLog"]
for f in key_funcs:
    if f"function {f}(" in html:
        print(f"  [OK] {f}()")
    else:
        print(f"  [MISSING] {f}()")