with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

checks = [
    ("nfc_scan in update_config", 'server.hasArg("nfc_scan")' in content),
    ("authMethodNFC set", "authMethodNFC = " in content),
    ("NFC re-init on enable", "nfcManager.init()" in content),
    ("nfc_scan in status JSON", '"nfc_scan"' in content),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")