with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Fix 1: updateStatus BLE logic - check ble_scan first
old_ble = r"""var ba=d.ble_authorized||d.ble_auth_valid;
    var bs=d.ble_scanning;
    var blc=E('chipBle');
    if(ba){
      blc.querySelector('.val').innerHTML='\\u5DF2\\u8BA4\\u8BC1';
      blc.className='status-chip on';
    }else if(bs){
      blc.querySelector('.val').innerHTML='\\u626B\\u63CF\\u4E2D';
      blc.className='status-chip on';
    }else{
      var ls=d.ble_last_seen;
      if(ls>0){blc.querySelector('.val').innerHTML=ls+'s\\u524D';blc.className='status-chip off';}
      else{blc.querySelector('.val').innerHTML='\\u5F85\\u673A';blc.className='status-chip off';}
    }"""

new_ble = r"""var blc=E('chipBle');
    if(!d.ble_scan){
      blc.querySelector('.val').innerHTML='\\u5DF2\\u5173\\u95ED';
      blc.className='status-chip off';
    }else if(d.ble_authorized||d.ble_auth_valid){
      blc.querySelector('.val').innerHTML='\\u5DF2\\u8BA4\\u8BC1';
      blc.className='status-chip on';
    }else if(d.ble_scanning){
      blc.querySelector('.val').innerHTML='\\u626B\\u63CF\\u4E2D';
      blc.className='status-chip on';
    }else if(d.ble_last_seen>0){
      blc.querySelector('.val').innerHTML=d.ble_last_seen+'s\\u524D';
      blc.className='status-chip off';
    }else{
      blc.querySelector('.val').innerHTML='\\u5F85\\u673A';
      blc.className='status-chip off';
    }"""

html = html.replace(old_ble, new_ble)

# Fix 2: Remove BLE chip update from updatePairStatus (it overrides status)
# Find the paired branch in updatePairStatus and remove the chipBle update
old_pair_update = r"""var blc=E('chipBle');if(blc&&!blc.querySelector('.val').innerHTML.match('\\u5DF2\\u8BA4\\u8BC1')){
        blc.querySelector('.val').innerHTML='\\u5DF2\\u914D\\u5BF9';blc.className='status-chip on';}"""

html = html.replace(old_pair_update, "")

# Also fix NFC status to check nfc_scan
old_nfc = r"""var nc=E('chipNfc');
    var na=d.nfc_scan!==false;
    if(cl){
      nc.querySelector('.val').innerHTML='\\u5DF2\\u9501\\u5B9A';
      nc.className='status-chip warn';
    }else if(!na){
      nc.querySelector('.val').innerHTML='\\u5DF2\\u5173\\u95ED';
      nc.className='status-chip off';
    }else{
      nc.querySelector('.val').innerHTML='\\u5DF2\\u89E3\\u9501';
      nc.className='status-chip on';
    }"""

new_nfc = r"""var nc=E('chipNfc');
    if(!d.nfc_scan){
      nc.querySelector('.val').innerHTML='\\u5DF2\\u5173\\u95ED';
      nc.className='status-chip off';
    }else if(cl){
      nc.querySelector('.val').innerHTML='\\u5DF2\\u9501\\u5B9A';
      nc.className='status-chip warn';
    }else{
      nc.querySelector('.val').innerHTML='\\u5DF2\\u89E3\\u9501';
      nc.className='status-chip on';
    }"""

html = html.replace(old_nfc, new_nfc)

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
checks = [
    ("BLE check ble_scan", "!d.ble_scan" in html),
    ("BLE disabled text", "\\u5DF2\\u5173\\u95ED" in html),
    ("NFC check nfc_scan", "!d.nfc_scan" in html),
    ("No pair chipBle override", "var blc=E('chipBle');if(blc&&!blc.querySelector('.val')" not in html),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")