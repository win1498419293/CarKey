import os, glob

src_dir = r"D:\CarKey_V5"
patterns = ["src/*.cpp", "src/*.h", "include/*.h"]

# Common garbled patterns from GBK misinterpreted as something else
# High-byte chars that appear as garbled
garbled_count = 0
files_with_garbled = []

for pattern in patterns:
    for f in glob.glob(os.path.join(src_dir, pattern)):
        with open(f, "rb") as fh:
            raw = fh.read()
        
        # Count bytes > 127 (non-ASCII, likely garbled Chinese)
        non_ascii = [b for b in raw if b > 127]
        if non_ascii:
            garbled_count += len(non_ascii)
            files_with_garbled.append((f, len(non_ascii)))

for f, count in sorted(files_with_garbled, key=lambda x: -x[1]):
    print(f"{count:5d} non-ASCII bytes: {os.path.basename(f)}")

print(f"\nTotal: {garbled_count} non-ASCII bytes in {len(files_with_garbled)} files")