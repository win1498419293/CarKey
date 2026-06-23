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

print(f"Generated {len(output)} lines, {len(html)} bytes HTML")