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
        # Check for double-escaping: if content still has \\, unescape again
        if '\\\\' in content:
            content = unescape_c_string(content)
        html += content + '\n'
    elif raw_line.startswith('"') and raw_line.endswith('"'):
        inner = raw_line[1:-1]
        content = unescape_c_string(inner)
        if '\\\\' in content:
            content = unescape_c_string(content)
        html += content

print(f"HTML length: {len(html)}")
# Check last 80 chars
print(f"Last 80: {repr(html[-80:])}")

# Check for remaining \ sequences
import re
backslashes = [(m.start(), html[max(0,m.start()-5):m.end()+5]) for m in re.finditer(r'\\\\', html)]
if backslashes:
    print(f"Found {len(backslashes)} double-backslash sequences remaining")
    for pos, ctx in backslashes[:3]:
        print(f"  pos {pos}: {repr(ctx)}")
else:
    print("No double-backslash sequences found - HTML is clean!")

# Re-encode
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
