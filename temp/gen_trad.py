with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

print(f"HTML: {len(html)} bytes")

# Traditional PROGMEM format - each line wrapped in quotes with \n
lines = html.split("\n")
output = [
    "#pragma once",
    "#include <pgmspace.h>",
    "const char kEmbeddedIndexPage[] PROGMEM ="
]

for i, line in enumerate(lines):
    # Escape only backslash and double-quote for C strings
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

# Verify
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "rb") as f:
    raw = f.read()

non_ascii = sum(1 for b in raw if b > 127)
print(f"Generated {len(output)} lines, {len(result)} bytes, non-ASCII: {non_ascii}")

# Check first and last lines
print(f"Line 0: {output[0]}")
print(f"Line 3: {output[3][:80]}...")
print(f"Last content line: {output[-3][:80]}...")