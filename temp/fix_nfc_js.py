with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Fix 1: NFC status - use === false instead of !d.nfc_scan
old_nfc = r"""var nc=E('chipNfc');
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

new_nfc = r"""var nc=E('chipNfc');
    if(d.nfc_scan===false){
      nc.querySelector('.val').innerHTML='\\u5DF2\\u5173\\u95ED';
      nc.className='status-chip off';
    }else if(d.config_locked){
      nc.querySelector('.val').innerHTML='\\u5DF2\\u9501\\u5B9A';
      nc.className='status-chip warn';
    }else{
      nc.querySelector('.val').innerHTML='\\u5DF2\\u89E3\\u9501';
      nc.className='status-chip on';
    }"""

html = html.replace(old_nfc, new_nfc)

# Fix 2: BLE status - also use === false
old_ble = r"""var blc=E('chipBle');
    if(!d.ble_scan){
      blc.querySelector('.val').innerHTML='\\u5DF2\\u5173\\u95ED';
      blc.className='status-chip off';
    }else if(d.ble_authorized||d.ble_auth_valid){"""

new_ble = r"""var blc=E('chipBle');
    if(d.ble_scan===false){
      blc.querySelector('.val').innerHTML='\\u5DF2\\u5173\\u95ED';
      blc.className='status-chip off';
    }else if(d.ble_authorized||d.ble_auth_valid){"""

html = html.replace(old_ble, new_ble)

# Fix 3: openSettings - use proper defaults  
old_settings = r"""if(E('cfgNfcScan'))E('cfgNfcScan').checked=d.nfc_scan!==false;"""
new_settings = r"""if(E('cfgNfcScan'))E('cfgNfcScan').checked=d.nfc_scan!==false;
  if(E('cfgBleScan'))E('cfgBleScan').checked=d.ble_scan===true;"""
html = html.replace(old_settings, new_settings)

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
for check in ["d.nfc_scan===false", "d.ble_scan===false", "d.ble_scan===true"]:
    print(f"  {'[OK]' if check in html else '[MISSING]'} {check}")