import re
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

print(f"File size: {len(content)} bytes")
print(f"Starts with: {content[:60]}")

# Reconstruct HTML and verify
lines = re.findall(r"\"(.+?)\\n\"", content)
html = "\n".join(lines)
print(f"Reconstructed HTML: {len(html)} bytes")
print(f"Has header div: {'class=\"header\"' in html}")
print(f"Has settings: {'settingsOverlay' in html}")
print(f"Has auth overlay: {'authOverlay' in html}")
print(f"Has pair card: {'pairCard' in html}")
print(f"Script has updateStatus: {'function updateStatus()' in html}")

# Check some Chinese entities
if "&#25377;" in html:
    print("Chinese entity for dan (挡) present: YES")
if "&#25163;" in html:
    print("Chinese entity for shou (手) present: YES")