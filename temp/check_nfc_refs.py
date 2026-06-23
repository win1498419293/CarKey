with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Check all nfc_scan references
import re
refs = [(m.start(), content[m.start():m.start()+80]) for m in re.finditer(r'nfc_scan|authMethodNFC', content)]
print(f"Found {len(refs)} references to nfc_scan/authMethodNFC:")
for pos, ctx in refs:
    print(f"  pos {pos}: ...{ctx}...")

# Check the status JSON 
# Find all "ble_scan" in the status response
ble_refs = [(m.start(), m.end()) for m in re.finditer(r'"ble_scan"', content)]
print(f"\n'ble_scan' in status JSON: {len(ble_refs)} occurrences")
for pos, end in ble_refs:
    # Show the line
    line_start = content.rfind('\n', 0, pos) + 1
    line_end = content.find('\n', end)
    print(f"  Line: {content[line_start:line_end].strip()[:100]}")