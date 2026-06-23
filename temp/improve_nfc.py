with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    cfg = f.read()

# Improve sensitivity while keeping stable
cfg = cfg.replace("#define NFC_SCAN_WINDOW_MS 80U", "#define NFC_SCAN_WINDOW_MS 100U")
cfg = cfg.replace("#define NFC_SCAN_POLL_TIMEOUT_MS 20U", "#define NFC_SCAN_POLL_TIMEOUT_MS 40U")
# Reduce interval slightly for more frequent scans
cfg = cfg.replace("#define NFC_SCAN_INTERVAL_MS 120U", "#define NFC_SCAN_INTERVAL_MS 100U")

with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(cfg)

print("NFC sensitivity improvements:")
print("  SCAN_INTERVAL: 120ms -> 100ms (scan more often)")
print("  SCAN_WINDOW:   80ms -> 100ms (longer detection window)")
print("  POLL_TIMEOUT:  20ms -> 40ms  (more time for card response)")
print("")
print("Each cycle: 100ms window with 40ms timeout = 2-3 deep polls")
print("Gap between scans: 0ms (continuous - BLE paused during NFC)")