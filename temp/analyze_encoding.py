with open(r"D:\CarKey_V5\src\WebManager.cpp", "rb") as f:
    raw = f.read()

# Try different encodings to recover Chinese
# The garbled text is likely GBK bytes displayed as Latin-1 or similar
# Try GBK -> UTF-8 conversion
try:
    # Try treating raw as GBK and re-encoding to UTF-8
    text = raw.decode("gbk", errors="replace")
    # Find all Chinese characters
    import re
    chinese = re.findall(r'[\u4e00-\u9fff]+', text)
    if chinese:
        print("=== Found Chinese text (GBK decode) ===")
        for chunk in chinese[:20]:
            if len(chunk) > 2:
                print(f"  {chunk[:60]}")
except:
    print("GBK decode failed")

# Also try: the file might be UTF-8 but some chars are corrupted
# Let me look at the non-ASCII bytes directly
print("\n=== Non-ASCII byte sequences ===")
non_ascii_positions = []
for i, b in enumerate(raw):
    if b > 127:
        non_ascii_positions.append(i)

# Show first few sequences
i = 0
count = 0
while i < len(non_ascii_positions) and count < 10:
    start = non_ascii_positions[i]
    # Find end of this sequence
    end = start
    j = i
    while j < len(non_ascii_positions) and non_ascii_positions[j] == end:
        end += 1
        j += 1
    seq_len = end - start
    seq_bytes = raw[start:end]
    # Show context
    ctx_start = max(0, start - 20)
    ctx_end = min(len(raw), end + 20)
    print(f"  pos {start}: [{seq_len} bytes] {seq_bytes[:20]}... context: {raw[ctx_start:ctx_end]}")
    i = j
    count += 1