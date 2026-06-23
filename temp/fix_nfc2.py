with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Fix 1: NFC status - change !d.nfc_scan to d.nfc_scan===false
old = "if(!d.nfc_scan){"
new = "if(d.nfc_scan===false){"
html = html.replace(old, new)
print(f"NFC check: {'replaced' if old not in html else 'NOT replaced'}")

# Fix 2: BLE status - change !d.ble_scan to d.ble_scan===false  
old2 = "if(!d.ble_scan){"
new2 = "if(d.ble_scan===false){"
html = html.replace(old2, new2)
print(f"BLE check: {'replaced' if old2 not in html else 'NOT replaced'}")

# Fix 3: NFC block uses 'cl' but should use 'd.config_locked'
# Currently: }else if(cl){
# Change to: }else if(d.config_locked){
old3 = "}else if(cl){"
new3 = "}else if(d.config_locked){"
# But only in NFC context - let's check if it appears multiple times
count = html.count(old3)
print(f"Found {count} occurrences of '}}else if(cl){{'")

# Only replace the one in NFC context (after d.nfc_scan check)
idx = html.find("d.nfc_scan===false")
if idx > 0:
    idx2 = html.find("}else if(cl){", idx)
    if idx2 > 0 and idx2 < idx + 400:
        html = html[:idx2] + "}else if(d.config_locked){" + html[idx2+15:]
        print("Replaced NFC cl->d.config_locked")

# Fix 4: openSettings BLE toggle default
old4 = "if(E('cfgBleScan'))E('cfgBleScan').checked=d.ble_scan||false;"
new4 = "if(E('cfgBleScan'))E('cfgBleScan').checked=d.ble_scan===true;"
html = html.replace(old4, new4)
print(f"BLE setting default: {'replaced' if old4 not in html else 'NOT replaced'}")

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
checks = [
    ("NFC === false", "d.nfc_scan===false" in html),
    ("BLE === false", "d.ble_scan===false" in html),
    ("NFC use d.config_locked", "d.config_locked" in html),
    ("BLE setting === true", "d.ble_scan===true" in html),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")