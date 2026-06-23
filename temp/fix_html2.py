import re

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    content = f.read()

# Find updatePairStatus function
idx = content.find("function updatePairStatus()")
if idx < 0:
    print("updatePairStatus not found!")
    exit(1)

# Find end of function
end = content.find("\nfunction startPair()", idx)
if end < 0:
    end = content.find("function startPair()", idx)

old = content[idx:end]

new = '''function updatePairStatus(){
  fetch('/api/ble/pairing/status').then(function(r){return r.json()}).then(function(d){
    var btName = d.bt_name || 'Unknown';
    var devName = d.name && d.name.length > 0 ? d.name : 'None';
    var devMac = d.mac || '';
    if(d.pairing){
      E('pBadge').className='pair-status pair-ing';E('pBadge').innerText='Searching...';
      E('pInfo').innerHTML='Device: <b>'+btName+'</b><br>Search in phone Bluetooth';
      E('pStart').style.display='none';E('pStop').style.display='inline-block';
    }else if(d.paired){
      E('pBadge').className='pair-status pair-ok';E('pBadge').innerText='Paired';
      E('pInfo').innerHTML='Device: <b>'+btName+'</b><br>Paired: '+devName+' ('+devMac+')';
      E('pStart').style.display='inline-block';E('pStop').style.display='none';
    }else{
      E('pBadge').className='pair-status pair-none';E('pBadge').innerText='Not Paired';
      E('pInfo').innerHTML='Device: <b>'+btName+'</b><br>Tap Start to begin pairing';
      E('pStart').style.display='inline-block';E('pStop').style.display='none';
    }
  }).catch(function(){});
}'''

content = content[:idx] + new + content[end:]

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="utf-8", newline="") as f:
    f.write(content)
print("HTML updated to show bt_name")
