with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    cfg = f.read()

cfg = cfg.replace("#define NFC_SCAN_WINDOW_MS 100U", "#define NFC_SCAN_WINDOW_MS 80U")
cfg = cfg.replace("#define NFC_SCAN_POLL_TIMEOUT_MS 50U", "#define NFC_SCAN_POLL_TIMEOUT_MS 20U")

with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(cfg)

print("Restored original: WINDOW=80, POLL_TIMEOUT=20")