import os

src_files = []
for root, dirs, files in os.walk(r"D:\CarKey_V5"):
    dirs[:] = [d for d in dirs if d not in ('.pio', '.piohome', '.pio_cache', '.pio_cache2', '.pio_tmp', '.vscode', 'temp')]
    for f in files:
        if f.endswith(('.cpp', '.h')) and f != 'EmbeddedIndexPage.h':
            src_files.append(os.path.join(root, f))

total_non_ascii = 0
files_with = []
for fp in sorted(src_files):
    with open(fp, "rb") as f:
        raw = f.read()
    if raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]
    
    non_ascii = [b for b in raw if b > 127]
    if non_ascii:
        total_non_ascii += len(non_ascii)
        files_with.append((os.path.basename(fp), len(non_ascii)))

print(f"Remaining non-ASCII bytes: {total_non_ascii}")
if files_with:
    print("Files still with non-ASCII:")
    for name, count in files_with:
        print(f"  {name}: {count} bytes")
else:
    print("ALL CLEAN!")