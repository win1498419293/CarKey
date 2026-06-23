with open(r"D:\CarKey_V5\src\NFCManager.cpp", "r", encoding="utf-8") as f:
    nfc = f.read()

# Remove the SPI speed increase
old = "SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);\n        SPI.setFrequency(5000000);  // PN532 max SPI clock"
new = "SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);"
nfc = nfc.replace(old, new)

with open(r"D:\CarKey_V5\src\NFCManager.cpp", "w", encoding="utf-8") as f:
    f.write(nfc)
print("Reverted SPI to default speed")