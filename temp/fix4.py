with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    lines = f.readlines()

preamble = []
in_html = False
html_raw_lines = []
for line in lines:
    if not in_html:
        preamble.append(line.rstrip('\n\r'))
        if 'const char kEmbeddedIndexPage[] PROGMEM =' in line:
            in_html = True
        continue
    stripped = line.strip()
    if stripped == ';':
        break
    if stripped == '"\n"' or stripped == '"\\n"':
        continue
    html_raw_lines.append(stripped)

def unescape_c_string(s):
    result = []
    i = 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            if s[i+1] == '\\':
                result.append('\\')
                i += 2
            elif s[i+1] == '"':
                result.append('"')
                i += 2
            elif s[i+1] == 'n':
                result.append('\n')
                i += 2
            else:
                result.append(s[i])
                i += 1
        else:
            result.append(s[i])
            i += 1
    return ''.join(result)

html = ""
for raw_line in html_raw_lines:
    if raw_line.startswith('"') and raw_line.endswith('\\n"'):
        inner = raw_line[1:-3]
        content = unescape_c_string(inner)
        # Fix double-escaping
        if '\\\\' in content:
            content = unescape_c_string(content)
        html += content + '\n'
    elif raw_line.startswith('"') and raw_line.endswith('"'):
        inner = raw_line[1:-1]
        content = unescape_c_string(inner)
        if '\\\\' in content:
            content = unescape_c_string(content)
        html += content

# Fix trailing artifacts: remove \n" at the very end if present
if html.endswith('\\n"\n'):
    html = html[:-3]  # remove \n"
    print("Fixed trailing \\n\"")
elif html.endswith('\\n"'):
    html = html[:-3]
    print("Fixed trailing \\n\" (no final newline)")

# Also remove any trailing \n" before </html>
import re
html = re.sub(r'\\n"\s*$', '', html)
html = html.rstrip() + '\n'

print(f"HTML length: {len(html)}")
print(f"Last 60: {repr(html[-60:])}")

def escape_for_c(s):
    result = []
    for ch in s:
        if ch == '\\':
            result.append('\\\\')
        elif ch == '"':
            result.append('\\"')
        elif ch == '\n':
            result.append('\\n')
        else:
            result.append(ch)
    return ''.join(result)

output = []
for pline in preamble:
    output.append(pline)

html_lines = html.split('\n')
for i, line in enumerate(html_lines):
    escaped = escape_for_c(line)
    if i < len(html_lines) - 1:
        output.append('"' + escaped + '\\n"')

output.append('"\\n"')
output.append(';')
output.append('')

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="utf-8", newline='') as f:
    f.write('\n'.join(output))

print(f"Written {len(output)} lines")
