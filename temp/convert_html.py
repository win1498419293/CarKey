# -*- coding: utf-8 -*-
import sys, os

# Read HTML from file
with open(sys.argv[1], "r", encoding="utf-8") as f:
    html = f.read()

print(f"HTML length: {len(html)}")
non_ascii = sum(1 for c in html if ord(c) > 127)
print(f"Non-ASCII chars: {non_ascii}")

def char_to_entity(c):
    cp = ord(c)
    if cp > 127:
        return "&#x{:X};".format(cp)
    return c

ascii_html = "".join(char_to_entity(c) for c in html)
remaining = sum(1 for c in ascii_html if ord(c) > 127)
print(f"After conversion: {len(ascii_html)} bytes, non-ASCII: {remaining}")

lines = ascii_html.split("\n")
output = ["#pragma once", "#include <pgmspace.h>", "const char kEmbeddedIndexPage[] PROGMEM ="]
for i, line in enumerate(lines):
    escaped = line.replace("\\", "\\\\").replace('"', '\\"')
    if i < len(lines) - 1:
        output.append('"' + escaped + '\\n"')
    else:
        output.append('"' + escaped + '\\n"')
output.append('"\\n"')
output.append(";")
output.append("")

with open(sys.argv[2], "w", encoding="ascii", newline="") as f:
    f.write("\n".join(output))

print(f"Generated {len(output)} lines")