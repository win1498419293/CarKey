# Read WebManager.cpp and extract garbled comment/log lines with context
with open(r"D:\CarKey_V5\src\WebManager.cpp", "rb") as f:
    raw = f.read()

# Remove BOM
if raw[:3] == b'\xef\xbb\xbf':
    raw = raw[3:]

# Decode as UTF-8 (the file is UTF-8 but with garbled Chinese)
text = raw.decode("utf-8", errors="replace")

lines = text.split("\n")
garbled_lines = []
for i, line in enumerate(lines):
    if any(ord(c) > 127 for c in line):
        # Skip pure ASCII emoji-like chars
        garbled_lines.append((i+1, line.strip()[:120]))

# Show them with context (previous line for comments)
print(f"Found {len(garbled_lines)} lines with non-ASCII in WebManager.cpp")
for ln, line in garbled_lines[:30]:
    print(f"  L{ln}: {line}")