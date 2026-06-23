with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

# Find a line with "header"
for line in content.split("\n"):
    if "header" in line.lower():
        print(repr(line[:120]))
        break

# Reconstruct properly
import re
# Each line is: "...content...\n"
pattern = re.compile(r'^"(.+?)\\n"$')
reconstructed = []
for line in content.split("\n"):
    m = pattern.match(line)
    if m:
        reconstructed.append(m.group(1))

html = "\n".join(reconstructed)
print(f"\nReconstructed HTML: {len(html)} bytes")
print(f"Sample from HTML: {html[200:350]}")