with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    lines = f.readlines()

preamble = []
html_lines = []
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
            inner = stripped[1:]
            if inner.endswith('\\n"'):
                inner = inner[:-3]
            elif inner.endswith('\\n";'):
                inner = inner[:-4]
            elif inner.endswith('";'):
                inner = inner[:-2]
            elif inner.endswith('"'):
                inner = inner[:-1]
            # Unescape C string
            inner = inner.replace('\\\\', '\x00').replace('\\"', '\x01')
            inner = inner.replace('\x00', '\\').replace('\x01', '"')
            html_lines.append(inner)

# Show last few html_lines
for i in range(max(0, len(html_lines)-5), len(html_lines)):
    print(f"HTML line {i}: {repr(html_lines[i])}")

print(f"\nLast line repr: {repr(html_lines[-1])}")
print(f"Last line hex: {html_lines[-1].encode('utf-8').hex(' ')}")
