with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Check for BLE pairing card on main page
has_pair_card = "pairCard" in html
print(f"Has pairCard on main page: {has_pair_card}")

# Check BLE scan toggle in settings
has_ble_toggle = "cfgBleScan" in html
print(f"Has cfgBleScan toggle: {has_ble_toggle}")

# Check if BLE pairing is in settings
has_pair_in_settings = html.find("BLE") > 0 and html.find("\u914D\u5BF9") > 0
print(f"BLE pairing in content: {has_pair_in_settings}")

# Show the settings section
settings_start = html.find("settingsOverlay")
if settings_start > 0:
    settings_html = html[settings_start:settings_start+2000]
    # Check for toggles
    if "cfgBleScan" in settings_html:
        print("cfgBleScan found in settings section")
    if "cfgNfcScan" in settings_html:
        print("cfgNfcScan found in settings section")
    if "startPair" in settings_html:
        print("startPair found in settings section")
    if "pStart" in settings_html:
        print("pStart found in settings section")

# Check the main page body for pairCard
body_end = html.find("<script>")
if body_end > 0:
    body = html[:body_end]
    if "pairCard" in body:
        print("\nWARNING: pairCard found in main page body!")
    if "pBadge" in body:
        # Check if pBadge is in settings or main
        settings_end = body.find("settingsOverlay")
        if settings_end > 0:
            before_settings = body[:settings_end]
            if "pBadge" in before_settings:
                print("WARNING: pBadge found BEFORE settings overlay (on main page)!")