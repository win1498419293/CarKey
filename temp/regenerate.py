with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

print(f"HTML: {len(html)} bytes")

lines = html.split("\n")
output = [
    "#pragma once",
    "#include <pgmspace.h>",
    "const char kEmbeddedIndexPage[] PROGMEM ="
]

for i, line in enumerate(lines):
    escaped = line.replace("\\", "\\\\").replace('"', '\\"')
    if i < len(lines) - 1:
        output.append('  "' + escaped + '\\n"')
    else:
        output.append('  "' + escaped + '\\n"')

output.append('  "\\n"')
output.append(";")
output.append("")

result = "\n".join(output)

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="ascii", newline="") as f:
    f.write(result)

print(f"Generated {len(output)} lines, {len(result)} bytes")

# Quick verify
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "rb") as f:
    raw = f.read()
non_ascii = sum(1 for b in raw if b > 127)
print(f"Non-ASCII bytes: {non_ascii}")

# Verify .innerText is gone
has_innerText = b'.innerText' in raw
has_innerHTML = b'.innerHTML' in raw
print(f"Has .innerText: {has_innerText}")
print(f"Has .innerHTML: {has_innerHTML}")