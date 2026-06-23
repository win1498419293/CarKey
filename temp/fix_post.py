with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find saveSettings and change query params to POST body
old_save = r"""function saveSettings(){
  var u='/api/update_config?new_ssid='+encodeURIComponent(E('cfgSsid').value)+'&new_bt='+encodeURIComponent(E('cfgBt').value);
  if(E('cfgPass').value)u+='&new_pass='+encodeURIComponent(E('cfgPass').value);
  if(E('cfgPwd').value)u+='&new_pwd='+encodeURIComponent(E('cfgPwd').value);
  u+='&sec_auth='+E('cfgSec').checked+'&ble_scan='+E('cfgBleScan').checked+'&nfc_scan='+E('cfgNfcScan').checked;
  fetch(u,{method:'POST'}).then(function(r){
    if(r.status===200){addLog('\u4FDD\u5B58\u6210\u529F','ok');closeSettings()}
    else{addLog('\u4FDD\u5B58\u5931\u8D25: '+r.status,'err')}
  }).catch(function(){addLog('\u7F51\u7EDC\u9519\u8BEF','err')});
}"""

new_save = r"""function saveSettings(){
  var b='new_ssid='+encodeURIComponent(E('cfgSsid').value)+'&new_bt='+encodeURIComponent(E('cfgBt').value);
  if(E('cfgPass').value)b+='&new_pass='+encodeURIComponent(E('cfgPass').value);
  if(E('cfgPwd').value)b+='&new_pwd='+encodeURIComponent(E('cfgPwd').value);
  b+='&sec_auth='+E('cfgSec').checked+'&ble_scan='+E('cfgBleScan').checked+'&nfc_scan='+E('cfgNfcScan').checked;
  fetch('/api/update_config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b}).then(function(r){
    if(r.status===200){addLog('\u4FDD\u5B58\u6210\u529F','ok');closeSettings()}
    else{addLog('\u4FDD\u5B58\u5931\u8D25: '+r.status,'err')}
  }).catch(function(){addLog('\u7F51\u7EDC\u9519\u8BEF','err')});
}"""

html = html.replace(old_save, new_save)

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

print("Changed to POST body" if "method:'POST',headers" in html else "FAILED")