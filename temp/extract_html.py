import re

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    content = f.read()

# Find start and end of PROGMEM string
start = content.find('"<!DOCTYPE html>')
end = content.rfind('</html>')

if start == -1 or end == -1:
    print("ERROR: Could not find HTML boundaries")
    exit(1)

# Extract the PROGMEM lines between start and end+len("</html>")
# We'll find the closing quote after </html>
end_marker = content.find('\n";', end)
if end_marker == -1:
    end_marker = content.find('\n"', end)
html_section = content[start:end_marker]

print(f"HTML section length: {len(html_section)}")

# Now we want to modify this. Let's take a different approach.
# Read line by line and process each PROGMEM line.

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    lines = f.readlines()

# Find PROGMEM lines
html_lines = []
preamble = []
postamble = []
state = "preamble"
for line in lines:
    if state == "preamble":
        preamble.append(line)
        if 'const char kEmbeddedIndexPage[] PROGMEM =' in line:
            state = "html"
        continue
    if state == "html":
        # Check if this is the end of the PROGMEM
        stripped = line.strip()
        if stripped == ';' or stripped == '"\n"':
            postamble.append(line)
            state = "postamble"
            continue
        # Extract the content: "content\n"
        # Format: "escaped_content\n"
        if stripped.startswith('"') and stripped.endswith('\\n"'):
            # Remove outer quotes and trailing \n"
            inner = stripped[1:-3]  # remove " at start, \n" at end
            # Unescape C string escapes
            inner = inner.replace('\\\\', '\\').replace('\\"', '"')
            html_lines.append(inner)
        elif stripped.startswith('"') and (stripped.endswith('"') or stripped.endswith('";')):
            inner = stripped[1:]
            if inner.endswith('";'):
                inner = inner[:-2]
            elif inner.endswith('"'):
                inner = inner[:-1]
            inner = inner.replace('\\\\', '\\').replace('\\"', '"')
            html_lines.append(inner)
        continue
    if state == "postamble":
        postamble.append(line)

html = '\n'.join(html_lines)
print(f"Extracted HTML: {len(html)} chars, {len(html_lines)} lines")
print(f"First 100: {html[:100]}")
print(f"Last 100: {html[-100:]}")
