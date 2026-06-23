import os

def show_garbled_lines(filepath):
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
            clean = ''.join(c if ord(c) < 128 else '?' for c in line)
            results.append((i+1, clean[:120]))
    return results

src_files = []
for root, dirs, files in os.walk(r"D:\CarKey_V5"):
    dirs[:] = [d for d in dirs if d not in ('.pio', '.piohome', '.pio_cache', '.pio_cache2', '.pio_tmp', '.vscode', 'temp')]
    for f in files:
        if f.endswith(('.cpp', '.h')) and f != 'EmbeddedIndexPage.h':
            src_files.append(os.path.join(root, f))

for fp in sorted(src_files):
    garbled = show_garbled_lines(fp)
    if garbled:
        fname = os.path.basename(fp)
        print(f"\n=== {fname} ({len(garbled)} lines) ===")
        for ln, line in garbled:
            print(f"  L{ln}: {line}")