import re

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    lines = f.readlines()

html_lines = []
state = "preamble"
for line in lines:
    if state == "preamble":
        if "const char kEmbeddedIndexPage[] PROGMEM =" in line:
            state = "html"
        continue
    if state == "html":
        stripped = line.strip()
        if stripped == ";":
            break
        if stripped == '"\\n"' or stripped == '"\n"':
            continue
        if stripped.startswith('"') and len(stripped) > 2:
            inner = stripped[1:]  # remove leading "
            # remove trailing \n" or \n"; or "; or "
            if inner.endswith('\\n"'):
                inner = inner[:-3]
            elif inner.endswith('\\n";'):
                inner = inner[:-4]
            elif inner.endswith('";'):
                inner = inner[:-2]
            elif inner.endswith('"'):
                inner = inner[:-1]
            # Unescape C string
            result = []
            i = 0
            while i < len(inner):
                if inner[i] == '\\' and i + 1 < len(inner):
                    nxt = inner[i+1]
                    if nxt == '\\':
                        result.append('\\')
                        i += 2
                    elif nxt == '"':
                        result.append('"')
                        i += 2
                    elif nxt == 'n':
                        result.append('\n')
                        i += 2
                    else:
                        result.append(inner[i])
                        i += 1
                else:
                    result.append(inner[i])
                    i += 1
            html_lines.append(''.join(result))

html = '\n'.join(html_lines)

# Fix: strip everything after </html>
idx = html.rfind('</html>')
if idx >= 0:
    html = html[:idx + 7] + '\n'

# Remove any remaining double-escapes
html = html.replace('\\\\', '\\')

# Write to data directory
with open(r"D:\CarKey_V5\data\index.html", "w", encoding="utf-8", newline='') as f:
    f.write(html)

print(f"Written {len(html)} bytes to data/index.html")

# Also remove index.html.gz so SPIFFS uses the uncompressed version
import os
gz_path = r"D:\CarKey_V5\data\index.html.gz"
if os.path.exists(gz_path):
    os.remove(gz_path)
    print("Removed index.html.gz")
