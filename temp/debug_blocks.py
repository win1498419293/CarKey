with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find the exact BLE block
idx1 = html.find("var ba=d.ble_authorized")
idx2 = html.find("var nc=E('chipNfc')")
if idx1 >= 0 and idx2 > idx1:
    ble_block = html[idx1:idx2]
    print(f"BLE block length: {len(ble_block)}")
    print("First 100:", repr(ble_block[:100]))
    print("Last 100:", repr(ble_block[-100:]))

# Also check NFC block
idx3 = html.find("var nc=E('chipNfc')")
idx4 = html.find("}).catch(function(){});", idx3)
if idx3 >= 0 and idx4 > idx3:
    nfc_block = html[idx3:idx4]
    print(f"\nNFC block length: {len(nfc_block)}")
    print("Content:", repr(nfc_block[:200]))