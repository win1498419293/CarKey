with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find the BLE status section
idx = html.find("var ba=d.ble_authorized")
if idx >= 0:
    print("=== BLE status block ===")
    print(repr(html[idx:idx+350]))
else:
    # Try alternative
    idx = html.find("ble_authorized")
    if idx >= 0:
        print("=== BLE context ===")
        print(repr(html[idx-50:idx+200]))
    else:
        print("ble_authorized NOT FOUND in HTML!")