with open(r"D:\CarKey_V5\src\NFCManager.cpp", "r", encoding="utf-8") as f:
    nfc = f.read()

# 1. Increase SPI speed in init()
old_spi = "SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);"
new_spi = "SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);\n        SPI.setFrequency(5000000);  // PN532 max SPI clock"
nfc = nfc.replace(old_spi, new_spi)

with open(r"D:\CarKey_V5\src\NFCManager.cpp", "w", encoding="utf-8") as f:
    f.write(nfc)
print("SPI speed: default -> 5MHz")

# 2. Reduce settle delay
with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    cfg = f.read()
cfg = cfg.replace("#define NFC_SCAN_SETTLE_MS 8U", "#define NFC_SCAN_SETTLE_MS 4U")
with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(cfg)
print("Settle: 8ms -> 4ms")

# 3. Increase window slightly
cfg = cfg.replace("#define NFC_SCAN_WINDOW_MS 100U", "#define NFC_SCAN_WINDOW_MS 110U")
with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(cfg)
print("Window: 100ms -> 110ms")

print("\nPer scan cycle (120ms):")
print("  beginNFC ~2ms + reconfig ~5ms + settle 4ms + poll 2×44ms ≈ 99ms")
print("  SPI now 5× faster, SAMConfig and poll both benefit")