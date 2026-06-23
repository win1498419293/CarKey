import os, re

# Define replacements for each file
# Format: {filename: [(old_pattern, new_text), ...]}
# We use the garbled text as-is for matching

fixes = {}

# ===== WebManager.cpp =====
fixes[r"D:\CarKey_V5\src\WebManager.cpp"] = [
    # L39: bool g_wasUnlocked record
    ('bool g_wasUnlocked = false;  // \u951f\u65a4\u62b7\u951f\u89d2\u51e4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u53eb\u8bb9\u62b7\u951f\u89d2\u51e4\u62b7\u951f\u65a4\u62b7\u951f\u7f2b\u4f19\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7',
     'bool g_wasUnlocked = false;  // Track previous unlock state for user auth change detection'),
    
    # L41: WiFi max connected time
    ('120000;  // 60\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u4e00\u951f\u8f7f\uff4f\u62b7\u951f\u65a4\u62b7\u951f\u52ab\u8bb9\u62b7NFC\u951f\u4fa5\u9769\u62b7\u951f\u65a4\u62b7',
     '120000;  // 60s reconnect cycle, then yield to NFC'),
    
    # L155: WiFi reconnect wait
    ('10000) return;  // 10 \u951f\u65a4\u62b7\u951f\u65a4\u62b7\u5343\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7\u951f\u65a4\u62b7',
     '10000) return;  // 10 second cooldown between reconnect attempts'),
]

# Apply fixes
for filepath, replacements in fixes.items():
    with open(filepath, "rb") as f:
        raw = f.read()
    
    # Remove BOM
    if raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]
    
    text = raw.decode("utf-8", errors="replace")
    original = text
    
    for old, new in replacements:
        if old in text:
            text = text.replace(old, new)
            print(f"  Fixed: {filepath.split(chr(92))[-1]} - {new[:60]}...")
        else:
            print(f"  NOT FOUND: {old[:40]}...")
    
    if text != original:
        with open(filepath, "w", encoding="utf-8", newline="") as f:
            f.write(text)
        print(f"  Saved: {filepath.split(chr(92))[-1]}")