with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find saveSettings
idx = html.find("function saveSettings()")
end = html.find("function rebootDevice()")
if idx >= 0 and end > idx:
    print("=== saveSettings ===")
    print(repr(html[idx:end]))