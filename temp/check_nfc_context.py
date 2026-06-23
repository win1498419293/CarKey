with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Show context around the 3 nfc_scan occurrences
positions = [14735, 16595, 23857]
for pos in positions:
    ctx_start = max(0, pos - 200)
    ctx_end = min(len(content), pos + 100)
    print(f"=== Context around pos {pos} ===")
    print(content[ctx_start:ctx_end])
    print()