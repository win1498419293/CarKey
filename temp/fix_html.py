# Fix the double-escaping issue in EmbeddedIndexPage.h
# The current file has HTML that was modified but improperly re-encoded
# We need to: extract HTML, remove double-escaping, then properly re-encode

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    lines = f.readlines()

# Find PROGMEM declaration start
preamble = []
html_source_lines = []
state = "preamble"
for line in lines:
    if state == "preamble":
        preamble.append(line.rstrip('\n\r'))
        if 'const char kEmbeddedIndexPage[] PROGMEM =' in line:
            state = "html"
        continue
    if state == "html":
        stripped = line.strip()
        if stripped == ';':
            break
        if stripped == '"\n"' or stripped == '"\\n"':
            continue
        if stripped.startswith('"') and len(stripped) > 2:
            inner = stripped[1:]  # remove leading "
            # Remove trailing \n" or \n"; or just " or ";
            if inner.endswith('\\n"'):
                inner = inner[:-3]
            elif inner.endswith('\\n";'):
                inner = inner[:-4]
            elif inner.endswith('";'):
                inner = inner[:-2]
            elif inner.endswith('"'):
                inner = inner[:-1]
            # C string unescape: \\\\ -> \\, \\\" -> \", then \\ -> \, \" -> "
            # Because the file might be double-escaped, we need to check
            # If inner contains \\\\, it's double-escaped
            # Single-escaped: \\ -> \, \" -> "
            # Double-escaped: \\\\ -> \\, \\\" -> \", then \\ -> \, \" -> "
            inner = inner.replace('\\\\', '\x00').replace('\\"', '\x01')
            inner = inner.replace('\x00', '\\').replace('\x01', '"')
            html_source_lines.append(inner)

html = '\n'.join(html_source_lines)
print(f"HTML length: {len(html)}")
print(f"Has pairCard: {'pairCard' in html}")
print(f"Has updatePairStatus: {'updatePairStatus' in html}")
print(f"Has .pair-card: {'.pair-card' in html}")

# Now properly re-encode for C++ PROGMEM
output = []
for pline in preamble:
    output.append(pline)

for line in html.split('\n'):
    # C string escape: \ -> \\, " -> \"
    escaped = line.replace('\\', '\\\\').replace('"', '\\"')
    output.append('"' + escaped + '\\n"')

output.append('"\\n"')
output.append(';')
output.append('')

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="utf-8", newline='') as f:
    f.write('\n'.join(output))

print(f"Written {len(output)} lines")
