with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find and remove the pairCard div from the main page
# It's between the action grid and the log card
old_pair_card_start = '<div class="card" id="pairCard">'
old_pair_card_end = '</div>\n<div class="card">\n  <div class="log-header">'

idx1 = html.find(old_pair_card_start)
if idx1 >= 0:
    # Find the matching closing </div> for this card
    # The pairCard section ends before the log card
    log_start = html.find('<div class="log-header">', idx1)
    if log_start > 0:
        # Find the </div> just before the log card
        # Look backwards from log_start
        div_end = html.rfind('</div>', idx1, log_start)
        if div_end > 0:
            # Remove from pairCard start to after its closing </div>
            # Need to find the exact end - it should be </div>\n</div> where the outer </div> closes the card
            section = html[idx1:log_start]
            # The section should end with </div>\n</div>
            last_div = section.rfind('</div>')
            if last_div > 0:
                html = html[:idx1] + html[idx1 + last_div + 6:]
                print(f"Removed pairCard from main page (removed {last_div+6} chars)")
            else:
                print("Could not find end of pairCard")
        else:
            print("Could not find div end before log")
    else:
        print("Log header not found")
else:
    print("pairCard not found on main page")

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
has_pair_main = False
body_end = html.find("<script>")
if body_end > 0:
    body = html[:body_end]
    settings_start = body.find("settingsOverlay")
    if settings_start > 0:
        main = body[:settings_start]
        has_pair_main = "pairCard" in main

print(f"pairCard on main page after fix: {has_pair_main}")
print(f"BLE pair in settings: {'startPair' in html}")
print(f"cfgBleScan toggle: {'cfgBleScan' in html}")
print(f"cfgNfcScan toggle: {'cfgNfcScan' in html}")