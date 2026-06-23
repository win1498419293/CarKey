import re

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    content = f.read()

# Find updatePairStatus function and add ble_authorized/ble_last_seen display
old = """function updatePairStatus(){
  fetch('/api/ble/pairing/status').then(function(r){return r.json()}).then(function(d){
    var dn=d.bt_name||'Unknown';
    E('pDevName').innerText='Device: '+dn;
    if(d.pairing){
      E('pBadge').className='pair-status pair-ing';E('pBadge').innerText='Searching...';
      E('pInfo').innerHTML='Search for <b>'+dn+'</b> in phone Bluetooth (GPS on!)';
      E('pStart').style.display='none';E('pStop').style.display='inline-block';
    }else if(d.paired){
      E('pBadge').className='pair-status pair-ok';E('pBadge').innerText='Paired';
      E('pInfo').innerHTML='Phone: '+d.name+' ('+d.mac+')';
      E('pStart').style.display='inline-block';E('pStop').style.display='none';
    }else{
      E('pBadge').className='pair-status pair-none';E('pBadge').innerText='Not Paired';
      E('pInfo').innerText='Click Start to begin';
      E('pStart').style.display='inline-block';E('pStop').style.display='none';
    }
  }).catch(function(){});
}"""

new = """function updatePairStatus(){
  fetch('/api/ble/pairing/status').then(function(r){return r.json()}).then(function(d){
    var dn=d.bt_name||'Unknown';
    E('pDevName').innerText='Device: '+dn;
    if(d.pairing){
      E('pBadge').className='pair-status pair-ing';E('pBadge').innerText='Searching...';
      E('pInfo').innerHTML='Search for <b>'+dn+'</b> in phone Bluetooth (GPS on!)';
      E('pStart').style.display='none';E('pStop').style.display='inline-block';
    }else if(d.paired){
      E('pBadge').className='pair-status pair-ok';E('pBadge').innerText='Paired';
      E('pInfo').innerHTML='Phone: '+d.name+' ('+d.mac+')';
      E('pStart').style.display='inline-block';E('pStop').style.display='none';
    }else{
      E('pBadge').className='pair-status pair-none';E('pBadge').innerText='Not Paired';
      E('pInfo').innerText='Click Start to begin';
      E('pStart').style.display='inline-block';E('pStop').style.display='none';
    }
  }).catch(function(){});
  // Also check BLE auth status from /api/status
  fetch('/api/status').then(function(r){return r.json()}).then(function(s){
    var authEl = E('bleAuthStatus');
    if(authEl){
      if(s.ble_authorized){
        authEl.innerHTML='<span style="color:var(--green)">AUTHORIZED</span> ('+s.ble_last_seen+'s ago)';
      }else if(s.ble_last_seen>=0){
        authEl.innerHTML='<span style="color:var(--sub)">Last seen '+s.ble_last_seen+'s ago</span>';
      }else{
        authEl.innerHTML='<span style="color:var(--sub)">Scanning...</span>';
      }
    }
  }).catch(function(){});
}"""

content = content.replace(old, new, 1)

# Add auth status display element after pair-info in the HTML body
old2 = '<div class="pair-info" id="pInfo">Click Start to begin</div>'
new2 = '<div class="pair-info" id="pInfo">Click Start to begin</div>\n<div class="pair-info" id="bleAuthStatus" style="margin-top:2px"></div>'
content = content.replace(old2, new2, 1)

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("UI updated with BLE auth status")
