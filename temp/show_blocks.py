with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find the NFC block
idx = html.find("var nc=E('chipNfc')")
if idx >= 0:
    print("=== NFC block ===")
    print(repr(html[idx:idx+300]))
else:
    print("NFC block not found")

print()

# Find BLE block
idx = html.find("var blc=E('chipBle')")
if idx >= 0:
    print("=== BLE block ===")
    print(repr(html[idx:idx+400]))
else:
    print("BLE block not found")