with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Fix: add nfc_scan to saveSettings URL
old = r"u+='&sec_auth='+E('cfgSec').checked+'&ble_scan='+E('cfgBleScan').checked;"
new = r"u+='&sec_auth='+E('cfgSec').checked+'&ble_scan='+E('cfgBleScan').checked+'&nfc_scan='+E('cfgNfcScan').checked;"

if old in html:
    html = html.replace(old, new)
    print("Added nfc_scan to saveSettings URL")
else:
    print("NOT FOUND - trying alternative")
    # Try with different spacing
    if "ble_scan=" in html:
        idx = html.find("ble_scan=")
        ctx = html[idx-30:idx+60]
        print(f"Context: {repr(ctx)}")

with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
if "nfc_scan=" in html and "cfgNfcScan" in html:
    print("nfc_scan present in saveSettings: YES")
else:
    print("nfc_scan present in saveSettings: NO")