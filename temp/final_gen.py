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

checks = [
    ("toggleEngine", "toggleEngine" in content),
    ("stop_engine", "stop_engine" in content),
    ("_lastEngine", "_lastEngine" in content),
    ("_lastLocked", "_lastLocked" in content),
    ("authModalTitle", "authModalTitle" in content),
    ("authSubmitBtn", "authSubmitBtn" in content),
    (".innerHTML", ".innerHTML" in content),
    (".innerText", ".innerText" in content),
]
print(f"Generated {len(output)} lines")
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")