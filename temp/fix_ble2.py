with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Replace BLE block (from var ba=... through var cl=...)
old_ble_start = "var ba=d.ble_authorized"
old_ble_end = "var cl=d.config_locked;"

idx1 = html.find(old_ble_start)
idx2 = html.find(old_ble_end, idx1)
if idx1 >= 0 and idx2 > idx1:
    new_ble = r"""var blc=E('chipBle');
    if(!d.ble_scan){
      blc.querySelector('.val').innerHTML='\u5DF2\u5173\u95ED';
      blc.className='status-chip off';
    }else if(d.ble_authorized||d.ble_auth_valid){
      blc.querySelector('.val').innerHTML='\u5DF2\u8BA4\u8BC1';
      blc.className='status-chip on';
    }else if(d.ble_scanning){
      blc.querySelector('.val').innerHTML='\u626B\u63CF\u4E2D';
      blc.className='status-chip on';
    }else if(d.ble_last_seen>0){
      blc.querySelector('.val').innerHTML=d.ble_last_seen+'s\u524D';
      blc.className='status-chip off';
    }else{
      blc.querySelector('.val').innerHTML='\u5F85\u673A';
      blc.className='status-chip off';
    }
    """
    html = html[:idx1] + new_ble + html[idx2:]
    print("Replaced BLE block")
else:
    print(f"BLE block not found: idx1={idx1}, idx2={idx2}")

# Replace NFC block (from var nc=E('chipNfc') through }).catch
old_nfc_start = "var nc=E('chipNfc');"
old_nfc_end = "}).catch(function(){});"

idx3 = html.find(old_nfc_start)
idx4 = html.find(old_nfc_end, idx3)
if idx3 >= 0 and idx4 > idx3:
    new_nfc = r"""var nc=E('chipNfc');
    if(!d.nfc_scan){
      nc.querySelector('.val').innerHTML='\u5DF2\u5173\u95ED';
      nc.className='status-chip off';
    }else if(cl){
      nc.querySelector('.val').innerHTML='\u5DF2\u9501\u5B9A';
      nc.className='status-chip warn';
    }else{
      nc.querySelector('.val').innerHTML='\u5DF2\u89E3\u9501';
      nc.className='status-chip on';
    }
  """
    html = html[:idx3] + new_nfc + html[idx4:]
    print("Replaced NFC block")
else:
    print(f"NFC block not found: idx3={idx3}, idx4={idx4}")

# Also fix updatePairStatus - remove BLE chip override
old_pair = r"var blc=E('chipBle');if(blc&&!blc.querySelector('.val').innerHTML.match('\u5DF2\u8BA4\u8BC1')){\n        blc.querySelector('.val').innerHTML='\u5DF2\u914D\u5BF9';blc.className='status-chip on';}"
if old_pair in html:
    html = html.replace(old_pair, "")
    print("Removed pair BLE chip override")
else:
    print("Pair BLE override not found")
    # Try alternative format
    if "chipBle" in html:
        # Find in updatePairStatus
        ps_idx = html.find("function updatePairStatus()")
        ps_end = html.find("function startPair()")
        if ps_idx >= 0 and ps_end > ps_idx:
            ps_block = html[ps_idx:ps_end]
            if "chipBle" in ps_block:
                print("Found chipBle in updatePairStatus, removing...")
                # Find and remove the line
                cb_idx = ps_block.find("var blc=E('chipBle')")
                cb_end = ps_block.find(";}", cb_idx)
                if cb_idx >= 0 and cb_end > cb_idx:
                    full_cb = ps_block[cb_idx:cb_end+2]
                    html = html.replace(full_cb, "")
                    print(f"Removed: {full_cb[:60]}...")

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
checks = [
    ("BLE check ble_scan", "!d.ble_scan" in html),
    ("BLE disabled text", "\\u5DF2\\u5173\\u95ED" in html),
    ("NFC check nfc_scan", "!d.nfc_scan" in html),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")