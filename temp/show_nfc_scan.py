with open(r"D:\CarKey_V5\src\TaskManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Find the NFC scan burst section
idx = content.find("NFC scan burst")
if idx > 0:
    # Show the scan section
    section = content[idx-50:idx+700]
    print("=== Current NFC scan logic ===")
    print(section)