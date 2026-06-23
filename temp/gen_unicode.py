with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

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

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="ascii", newline="") as f:
    f.write("\n".join(output))

# Verify
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

print(f"Generated {len(output)} lines")
print(f"Has JS unicode escapes: {'\\\\u' in content}")
print(f"Has HTML entities (body): {'&#' in content}")
print(f"No HTML entities in \\uXXXX: {'&#\\\\u' not in content}")

# Check specific patterns
checks = [
    ("confirm with unicode", "confirm('\\\\u" in content),
    ("BLE chip update from pair", "chipBle" in content),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")