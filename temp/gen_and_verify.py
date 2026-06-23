with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

lines = html.split("\n")
output = [
    "#pragma once",
    "#include <pgmspace.h>",
    "const char kEmbeddedIndexPage[] PROGMEM ="
]
for i, line in enumerate(lines):
    escaped = line.replace("\\", "\\\\").replace('"', '\\"')
    if i < len(lines) - 1:
        output.append('  "' + escaped + '\\n"')
    else:
        output.append('  "' + escaped + '\\n"')
output.append('  "\\n"')
output.append(";")
output.append("")

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="ascii", newline="") as f:
    f.write("\n".join(output))

# Final verification
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    c = f.read()

checks = [
    ("nfc_scan in saveSettings", "nfc_scan=" in c and "cfgNfcScan" in c),
    ("NFC === false", "d.nfc_scan===false" in c),
    ("BLE === false", "d.ble_scan===false" in c),
    ("cfgNfcScan toggle", "cfgNfcScan" in c),
    ("cfgBleScan toggle", "cfgBleScan" in c),
    ("startPair in settings", "startPair" in c),
]
all_ok = True
for name, ok in checks:
    if not ok: all_ok = False
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")

# Also verify C++ logging
with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    cpp = f.read()
cpp_checks = [
    ("Load log", "Loaded auth_nfc" in cpp),
    ("Save log", "Saving auth_nfc" in cpp),
]
for name, ok in cpp_checks:
    if not ok: all_ok = False
    print(f"  {'[OK]' if ok else '[MISSING]'} C++: {name}")

print(f"\nGenerated {len(output)} lines, All: {'PASS' if all_ok else 'FAIL'}")