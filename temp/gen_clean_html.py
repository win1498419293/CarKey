# Generate a clean EmbeddedIndexPage.h from the current one
# by extracting HTML, removing pairing changes, and checking for errors

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    lines = f.readlines()

preamble = []
html_raw = []
state = "preamble"
for line in lines:
    if state == "preamble":
        preamble.append(line.rstrip('\n\r'))
        if "const char kEmbeddedIndexPage[] PROGMEM =" in line:
            state = "html"
        continue
    stripped = line.strip()
    if stripped == ";":
        break
    if stripped == '"\\n"' or stripped == '"\n"':
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
        result = []
        i = 0
        while i < len(inner):
            if inner[i] == '\\' and i + 1 < len(inner):
                nxt = inner[i+1]
                if nxt == '\\': result.append('\\'); i += 2
                elif nxt == '"': result.append('"'); i += 2
                elif nxt == 'n': result.append('\n'); i += 2
                else: result.append(inner[i]); i += 1
            else:
                result.append(inner[i]); i += 1
        html_raw.append(''.join(result))

html = '\n'.join(html_raw)

# Clean: remove pairing CSS
import re
# Remove the pairing CSS block (between .footer and </style>)
html = re.sub(r'\.footer.*?\.pair-btn-ghost.*?\}', '.footer { text-align: center; font-size: 10px; color: var(--sub); margin-top: 20px; padding: 10px 0; }', html, flags=re.DOTALL)

# Remove pairing card HTML
html = re.sub(r'<div class="pair-card".*?</div>\s*\n', '', html, flags=re.DOTALL)

# Remove pairing JS functions  
html = re.sub(r'function updatePairStatus\(\) \{.*?\n\}\n', '', html, flags=re.DOTALL)
html = re.sub(r'function startBlePairing\(\) \{.*?\n\}\n', '', html, flags=re.DOTALL)
html = re.sub(r'function stopBlePairing\(\) \{.*?\n\}\n', '', html, flags=re.DOTALL)
html = re.sub(r'function clearBlePairing\(\) \{.*?\n\}\n', '', html, flags=re.DOTALL)

# Remove setInterval for pairing
html = html.replace('setInterval(updatePairStatus, 3000);\n', '')
html = html.replace('updatePairStatus();\n', '')

# Also fix setInterval for updateStatus to be after fetchLogs (like original)
# Already fine

# Truncate at </html>
idx = html.rfind('</html>')
if idx >= 0:
    html = html[:idx + 7] + '\n'

# Re-encode to PROGMEM
def escape_c(s):
    r = []
    for c in s:
        if c == '\\': r.append('\\\\')
        elif c == '"': r.append('\\"')
        elif c == '\n': r.append('\\n')
        else: r.append(c)
    return ''.join(r)

out = list(preamble)
for line in html.split('\n'):
    out.append('"' + escape_c(line) + '\\n"')

out.append('"\\n"')
out.append(';')
out.append('')

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="utf-8", newline='') as f:
    f.write('\n'.join(out))

print(f"Generated clean HTML: {len(html)} bytes, {len(out)} lines")
print(f"Has pairCard: {'pairCard' in html}")
print(f"Has updatePairStatus: {'updatePairStatus' in html}")
