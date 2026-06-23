with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    cfg = f.read()

# Better balance: keep interval same, increase window and timeout
cfg = cfg.replace("#define NFC_SCAN_INTERVAL_MS 100U", "#define NFC_SCAN_INTERVAL_MS 120U")

with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(cfg)

print("Final NFC scan timing:")
print("  INTERVAL:  120ms (same as original)")
print("  WINDOW:    100ms (+25%)")
print("  POLL:      40ms  (+100%)")
print("  Gap:       20ms for BLE")
print("  ~2-3 deep polls per window")