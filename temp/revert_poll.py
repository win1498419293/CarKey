with open(r"D:\CarKey_V5\include\Config.h", "r", encoding="utf-8") as f:
    cfg = f.read()

# Revert to working value
cfg = cfg.replace("#define NFC_SCAN_POLL_TIMEOUT_MS 50U", "#define NFC_SCAN_POLL_TIMEOUT_MS 40U")

with open(r"D:\CarKey_V5\include\Config.h", "w", encoding="utf-8") as f:
    f.write(cfg)

print("POLL_TIMEOUT: 50 -> 40 (verified working value)")
print("")
print("Final state (same as when NFC worked):")
print("  reconfig  + settle 8ms = every scan")
print("  WINDOW   = 100ms")
print("  POLL     = 40ms")
print("  INTERVAL = 120ms")
print("")
print("Per cycle: ~18ms overhead + ~2 polls × 42ms in 100ms window")