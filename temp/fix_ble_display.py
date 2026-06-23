with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find the updateStatus function and improve BLE display
# Add more BLE status detail
old_ble = """var ba=d.ble_authorized||d.ble_auth_valid;
    var blc=E('chipBle');blc.querySelector('.val').innerHTML=ba?'&#24050;&#35748;&#35777;':'&#26410;&#35748;&#35777;';
    blc.className='status-chip'+(ba?' on':' off');"""

new_ble = """var ba=d.ble_authorized||d.ble_auth_valid;
    var bs=d.ble_scanning;
    var blc=E('chipBle');
    if(ba){
      blc.querySelector('.val').innerHTML='&#24050;&#35748;&#35777;';
      blc.className='status-chip on';
    }else if(bs){
      blc.querySelector('.val').innerHTML='&#25195;&#25551;&#20013;';
      blc.className='status-chip on';
    }else{
      var ls=d.ble_last_seen;
      if(ls>0){blc.querySelector('.val').innerHTML=ls+'s&#21069;';blc.className='status-chip off';}
      else{blc.querySelector('.val').innerHTML='&#24453;&#26426;';blc.className='status-chip off';}
    }"""

html = html.replace(old_ble, new_ble)

# Also improve NFC status - show more info
old_nfc = """var cl=d.config_locked;
    var nc=E('chipNfc');nc.querySelector('.val').innerHTML=cl?'&#24050;&#38145;&#23450;':'&#24050;&#35299;&#38145;';
    nc.className='status-chip'+(cl?' warn':' on');"""

new_nfc = """var cl=d.config_locked;
    var nc=E('chipNfc');
    if(cl){
      nc.querySelector('.val').innerHTML='&#24050;&#38145;&#23450;';
      nc.className='status-chip warn';
    }else{
      nc.querySelector('.val').innerHTML='&#24050;&#35299;&#38145;';
      nc.className='status-chip on';
    }"""

html = html.replace(old_nfc, new_nfc)

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
checks = [
    ("BLE scanning status", "&#25195;&#25551;&#20013;" in html),
    ("BLE standby", "&#24453;&#26426;" in html),
    ("BLE last seen seconds", "ls+'s&#21069;'" in html),
    ("NFC status detal", "&#24050;&#35299;&#38145;" in html),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")