with open(r"D:\CarKey_V5\src\NFCManager.cpp", "r", encoding="utf-8") as f:
    nfc = f.read()

# Show init and poll functions
idx = nfc.find("void NFCManager::init()")
if idx > 0:
    print("=== init() ===")
    print(nfc[idx:idx+400])
    print()

idx = nfc.find("void NFCManager::reconfig()")
if idx > 0:
    print("=== reconfig() ===")
    print(nfc[idx:idx+100])

# Check SPI speed
print("\n=== SPI ===")
for line in nfc.split("\n"):
    if "SPI" in line and ("begin" in line or "clock" in line.lower() or "speed" in line.lower() or "freq" in line.lower()):
        print(f"  {line.strip()}")

# Check if there's a setSPISpeed or similar
if "setClock" in nfc or "setFrequency" in nfc or "SPISettings" in nfc:
    print("  Has SPI speed settings")
else:
    print("  NO custom SPI speed - using default")