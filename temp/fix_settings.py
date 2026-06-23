with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find the BLE scan toggle and add NFC toggle after it, plus BLE pairing section
old_ble_toggle = """    <div class="toggle-row">
      <span>BLE&#25195;&#25551;</span>
      <label class="toggle"><input type="checkbox" id="cfgBleScan"><span class="slider"></span></label>
    </div>
    <div class="btn-row">"""

# Add NFC toggle + BLE pairing section
new_section = """    <div class="toggle-row">
      <span>BLE&#25195;&#25551;</span>
      <label class="toggle"><input type="checkbox" id="cfgBleScan"><span class="slider"></span></label>
    </div>
    <div class="toggle-row">
      <span>NFC&#25195;&#25551;</span>
      <label class="toggle"><input type="checkbox" id="cfgNfcScan"><span class="slider"></span></label>
    </div>
    <div class="divider"></div>
    <div style="font-size:11px;color:var(--blue);font-weight:600;margin-bottom:6px">&#128225; BLE &#37197;&#23545;</div>
    <div id="pInfo" style="font-size:10px;color:var(--sub);margin:4px 0">&#28857;&#20987;&#24320;&#22987;&#37197;&#23545;&#65292;&#22312;&#25163;&#26426;&#34013;&#29273;&#20013;&#25628;&#32032;&#35774;&#22791;</div>
    <div style="display:flex;gap:6px;align-items:center">
      <button class="btn-sm" id="pStart" onclick="startPair()">&#24320;&#22987;&#37197;&#23545;</button>
      <button class="btn-sm" id="pStop" onclick="stopPair()" style="display:none">&#20572;&#27490;</button>
      <button class="btn-sm danger" onclick="clearPair()">&#28165;&#38500;</button>
      <span class="pair-status badge-none" id="pBadge">&#26410;&#37197;&#23545;</span>
    </div>
    <div class="divider"></div>
    <div class="btn-row">"""

if old_ble_toggle in html:
    html = html.replace(old_ble_toggle, new_section)
    print("Added NFC toggle + BLE pairing to settings")
else:
    print("BLE toggle not found in expected format")
    # Try to find it differently
    idx = html.find("cfgBleScan")
    if idx >= 0:
        print(f"Found cfgBleScan at {idx}: ...{html[idx-30:idx+80]}...")

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
checks = [
    ("cfgBleScan", "cfgBleScan" in html),
    ("cfgNfcScan", "cfgNfcScan" in html),
    ("pStart in settings", "pStart" in html),
    ("pBadge in settings", "pBadge" in html),
    ("divider in settings", "divider" in html),
    ("pairCard removed", "pairCard" not in html[:html.find("<script>")]),
]
all_ok = True
for name, ok in checks:
    status = "OK" if ok else "MISSING"
    if not ok: all_ok = False
    print(f"  [{status}] {name}")
print(f"\nAll checks: {'PASS' if all_ok else 'FAIL'}")