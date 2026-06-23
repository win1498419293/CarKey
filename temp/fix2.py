# Build the correct PROGMEM file from scratch
# Read current file, extract only the HTML modifications we want,
# then properly re-encode everything

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    lines = f.readlines()

# Get preamble (everything before the first HTML line)
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

# Now html_raw_lines are like: "escaped_content\n"
# We need to unescape properly. 
# Each line is a C string literal: "escaped\n"
# where \n at the end is the newline char in the final HTML

def unescape_c_string(s):
    """Unescape a C string literal content (between quotes).
    Handles: \\ -> \, \" -> ", \n -> newline
    """
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
    # Remove leading " and trailing \n" 
    if raw_line.startswith('"') and raw_line.endswith('\\n"'):
        inner = raw_line[1:-3]
        content = unescape_c_string(inner)
        html += content + '\n'
    elif raw_line.startswith('"') and raw_line.endswith('"'):
        inner = raw_line[1:-1]
        content = unescape_c_string(inner)
        html += content
    else:
        print(f"WARNING: Unexpected line format: {repr(raw_line)}")

print(f"HTML length: {len(html)}")
print(f"Last 50: {repr(html[-50:])}")

# Now properly re-encode for C++ PROGMEM
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

# Split HTML into lines and encode each
html_lines = html.split('\n')
for i, line in enumerate(html_lines):
    escaped = escape_for_c(line)
    if i < len(html_lines) - 1:
        output.append('"' + escaped + '\\n"')
    else:
        # Last line (should be empty since html ends with \n)
        if escaped:
            output.append('"' + escaped + '\\n"')
        # Don't add empty last line

output.append('"\\n"')
output.append(';')
output.append('')

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="utf-8", newline='') as f:
    f.write('\n'.join(output))

print(f"Written {len(output)} lines")
