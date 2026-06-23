with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "rb") as f:
    raw = f.read()

# Check for BOM
if raw[:3] == b'\xef\xbb\xbf':
    print("UTF-8 BOM detected - this could cause issues!")
else:
    print("No BOM - good")

# Check for any non-ASCII bytes
non_ascii = [i for i, b in enumerate(raw) if b > 127]
print(f"Non-ASCII bytes: {len(non_ascii)}")

# Check for any unusual control chars
unusual = [i for i, b in enumerate(raw) if b < 0x20 and b not in (0x0a, 0x0d, 0x09)]
print(f"Unusual control chars: {len(unusual)}")
if unusual:
    for idx in unusual[:5]:
        print(f"  pos {idx}: 0x{raw[idx]:02x}")

# Show bytes around position 7000 (should be near first entity)
print(f"\nBytes around 7190-7250:")
print(raw[7190:7250])
print(f"\nAs string: {raw[7190:7250].decode('ascii', errors='replace')}")