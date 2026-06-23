with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Show full saveSettings function
idx = html.find("function saveSettings()")
end = html.find("function rebootDevice()")
if idx >= 0 and end > idx:
    print(html[idx:end])
else:
    print("saveSettings NOT FOUND")

# Also check if nfc_scan is in the URL
if "nfc_scan=" in html:
    print("\n=== nfc_scan FOUND in save URL ===")
else:
    print("\n=== nfc_scan MISSING from save URL! ===")