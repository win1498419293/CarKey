import os, re

def show_garbled_lines(filepath):
    """Show all lines with non-ASCII chars and their line numbers."""
    with open(filepath, "rb") as f:
        raw = f.read()
    if raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]
    text = raw.decode("utf-8", errors="replace")
    lines = text.split("\n")
    
    results = []
    for i, line in enumerate(lines):
        non_ascii = [c for c in line if ord(c) > 127]
        if non_ascii:
            # Clean up for display - replace garbled chars
            clean = ''.join(c if ord(c) < 128 else '?' for c in line)
            results.append((i+1, clean[:100]))
    return results

# Check all C++ files
src_files = []
for root, dirs, files in os.walk(r"D:\CarKey_V5"):
    for f in files:
        if f.endswith(('.cpp', '.h')):
            src_files.append(os.path.join(root, f))

for fp in sorted(src_files):
    garbled = show_garbled_lines(fp)
    if garbled:
        fname = os.path.basename(fp)
        print(f"\n=== {fname} ({len(garbled)} lines) ===")
        for ln, line in garbled:
            print(f"  L{ln}: {line}")