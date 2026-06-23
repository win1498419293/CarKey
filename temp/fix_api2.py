import re

with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Find the pairing status API section
idx = content.find('/api/ble/pairing/status')
if idx < 0:
    print("API not found!")
    exit(1)

# Find the function body
start = content.rfind('server.on("', 0, idx)
end = content.find('});', idx) + 3

old = content[start:end]
print(f"Found API at offset {start}-{end}")

new = '''server.on("/api/ble/pairing/status", []() {
        sendNoCacheHeaders();
        String json = "{";
        json += "\\"bt_name\\":\\"" + jsonEscape(bt_name) + "\\",";
        json += "\\"pairing\\":" + String(bleManager.isPairing() ? "true" : "false") + ",";
        json += "\\"paired\\":" + String(bleManager.isPaired() ? "true" : "false") + ",";
        json += "\\"name\\":\\"" + jsonEscape(bleManager.getPairedDeviceName()) + "\\",";
        json += "\\"mac\\":"" + String("\\"") + bleManager.getPairedDeviceMac() + "\\"";
        json += "}";
        server.send(200, "application/json", json);
    });'''

content = content[:start] + new + content[end:]

with open(r"D:\CarKey_V5\src\WebManager.cpp", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("API updated with bt_name field")
