html = r"""<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>CarKey V5</title>
<style>
:root{--bg:#080c16;--card:#111827;--txt:#f8fafc;--sub:#64748b;--blue:#3b82f6;--green:#10b981;--red:#ef4444;--border:rgba(255,255,255,0.05)}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;outline:none;margin:0}
body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--bg);color:var(--txt);padding:16px;min-height:100vh}
.card{background:var(--card);border-radius:14px;padding:14px;margin-bottom:16px;border:1px solid var(--border)}
.header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}
.header h1{font-size:18px;font-weight:600;letter-spacing:2px}
.header span{font-weight:300;color:var(--sub)}
.engine-wrap{display:flex;justify-content:center;margin:30px 0;position:relative}
.engine-btn{width:120px;height:120px;border-radius:50%;background:linear-gradient(145deg,#1e293b,#0f172a);border:1px solid rgba(59,130,246,0.5);box-shadow:0 0 30px rgba(59,130,246,0.4);display:flex;flex-direction:column;align-items:center;justify-content:center;cursor:pointer;transition:0.3s}
.engine-btn:active{transform:scale(0.92)}
.engine-title{font-size:14px;font-weight:600;letter-spacing:1px}
.engine-sub{font-size:9px;color:var(--sub);margin-top:2px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:16px}
.act-card{display:flex;flex-direction:column;align-items:center;padding:16px 8px;cursor:pointer;background:rgba(255,255,255,0.02);border:1px solid var(--border);border-radius:10px}
.act-card:active{background:rgba(255,255,255,0.05)}
.act-title{font-size:12px;font-weight:500}
.act-sub{font-size:10px;color:var(--sub);margin-top:2px}
.log-section{margin-top:20px}
.log-list{display:flex;flex-direction:column;gap:4px;max-height:180px;overflow-y:auto}
.log-item{background:rgba(0,0,0,0.4);padding:6px 10px;border-radius:5px;font-size:10px;display:flex;gap:8px}
.log-time{color:var(--sub);white-space:nowrap}
.footer{text-align:center;font-size:10px;color:var(--sub);margin-top:20px;padding:10px 0}
.overlay{position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(8,12,22,0.95);display:none;justify-content:center;align-items:center;z-index:9999}
.modal-box{background:#0f172a;border:1px solid rgba(59,130,246,0.3);border-radius:14px;padding:20px;width:90%;max-width:360px}
.modal-title{font-size:14px;font-weight:600;margin-bottom:12px;display:flex;align-items:center;gap:8px}
.btn-row{display:flex;gap:8px;margin-top:14px;justify-content:flex-end}
.btn{padding:8px 16px;border-radius:8px;border:none;cursor:pointer;font-size:12px;font-weight:500;transition:0.2s}
.btn-primary{background:var(--blue);color:#fff}
.btn-danger{background:var(--red);color:#fff}
.btn-ghost{background:transparent;border:1px solid var(--border);color:var(--sub)}
label{display:block;font-size:11px;color:var(--sub);margin:10px 0 4px}
input,select{width:100%;padding:8px 10px;border-radius:8px;border:1px solid var(--border);background:rgba(0,0,0,0.3);color:var(--txt);font-size:13px}
.toggle-row{display:flex;justify-content:space-between;align-items:center;padding:6px 0;font-size:11px}
.nav-btn{font-size:20px;cursor:pointer;color:var(--sub);padding:4px}
.nav-btn:hover{color:var(--blue)}
#battery{text-align:center;font-size:10px;color:var(--sub);margin-bottom:8px}
.pair-card{background:var(--card);border-radius:12px;padding:12px 14px;margin-bottom:14px;border:1px solid var(--border)}
.pair-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:4px}
.pair-title{font-size:11px;color:var(--blue);letter-spacing:1px}
.pair-name{font-size:10px;color:var(--txt);margin-bottom:2px}
.pair-status{font-size:10px;padding:3px 8px;border-radius:10px}
.pair-none{background:rgba(100,116,139,0.2);color:var(--sub)}
.pair-ok{background:rgba(16,185,129,0.2);color:var(--green)}
.pair-ing{background:rgba(59,130,246,0.2);color:var(--blue);animation:pulse 1.5s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}
.pair-info{font-size:10px;color:var(--sub);margin:4px 0}
.pair-btn-sm{font-size:10px;padding:5px 10px;border-radius:6px;border:none;cursor:pointer;margin-right:4px}
</style>
</head>
<body>
<div class="header">
  <h1>CARKEY <span>V5</span></h1>
  <div><span class="nav-btn" onclick="openSettings()" title="Settings">⚙</span></div>
</div>
<div id="battery">--.- V</div>
<div class="engine-wrap">
  <div class="engine-btn" onclick="openAuth()">
    <div class="engine-title" id="engTitle">START</div>
    <div class="engine-sub" id="engSub">Click to start</div>
  </div>
</div>
<div class="grid">
  <div class="act-card" onclick="doApi('/api/unlock')"><div class="act-title">Unlock</div><div class="act-sub">NFC</div></div>
  <div class="act-card" onclick="doApi('/api/lock')"><div class="act-title">Lock</div><div class="act-sub">Doors</div></div>
  <div class="act-card" onclick="doApi('/api/horn')"><div class="act-title">Horn</div><div class="act-sub">Alert</div></div>
  <div class="act-card" onclick="doApi('/api/window')"><div class="act-title">Window</div><div class="act-sub">Toggle</div></div>
</div>
<div class="pair-card" id="pairCard">
  <div class="pair-header">
    <div class="pair-title">Bluetooth Pairing</div>
    <div class="pair-status pair-none" id="pBadge">Not Paired</div>
  </div>
  <div class="pair-name" id="pDevName">Device: --</div>
  <div class="pair-info" id="pInfo">Click Start to begin</div>
  <div style="margin-top:6px">
    <button class="pair-btn-sm btn-primary" id="pStart" onclick="startPair()">Start</button>
    <button class="pair-btn-sm btn-danger" id="pStop" onclick="stopPair()" style="display:none">Stop</button>
    <button class="pair-btn-sm btn-ghost" onclick="clearPair()">Clear</button>
  </div>
</div>
<div class="log-section">
  <div class="log-list" id="logList"></div>
</div>
<div class="footer">CarKey V5</div>
<div class="overlay" id="authOverlay">
  <div class="modal-box">
    <div class="modal-title">Start Password</div>
    <input type="password" id="authPwd" placeholder="6 digits" maxlength="6" autofocus>
    <div class="btn-row">
      <button class="btn btn-ghost" onclick="closeAuth()">Cancel</button>
      <button class="btn btn-primary" onclick="doStart()">Start Engine</button>
    </div>
    <div id="authMsg" style="font-size:11px;color:var(--red);margin-top:8px"></div>
  </div>
</div>
<div class="overlay" id="settingsOverlay">
  <div class="modal-box">
    <div class="modal-title">Settings</div>
    <label>WiFi SSID</label><input type="text" id="cfgSsid">
    <label>WiFi Password</label><input type="password" id="cfgPass">
    <label>Bluetooth Name</label><input type="text" id="cfgBt">
    <label>Start Password</label><input type="password" id="cfgPwd" maxlength="6">
    <div class="toggle-row"><span>BLE Scan</span><input type="checkbox" id="cfgBleScan"></div>
    <div class="toggle-row"><span>Security Auth</span><input type="checkbox" id="cfgSec"></div>
    <div class="btn-row">
      <button class="btn btn-ghost" onclick="closeSettings()">Cancel</button>
      <button class="btn btn-primary" onclick="saveSettings()">Save</button>
      <button class="btn btn-danger" onclick="rebootDevice()">Reboot</button>
    </div>
    <div id="settingsMsg" style="font-size:11px;color:var(--red);margin-top:8px"></div>
  </div>
</div>
<script>
var E=function(id){return document.getElementById(id)};
function addLog(m,t){
  var l=E('logList');if(!l)return;
  var d=new Date(),ts=('0'+d.getHours()).slice(-2)+':'+('0'+d.getMinutes()).slice(-2)+':'+('0'+d.getSeconds()).slice(-2);
  var c=t==='err'?'var(--red)':t==='ok'?'var(--green)':'var(--sub)';
  l.innerHTML='<div class="log-item"><span class="log-time">'+ts+'</span><span style="color:'+c+'">'+m+'</span></div>'+l.innerHTML;
  if(l.children.length>50)l.removeChild(l.lastChild);
}
function openAuth(){E('authOverlay').style.display='flex';E('authPwd').focus()}
function closeAuth(){E('authOverlay').style.display='none';E('authPwd').value='';E('authMsg').innerText=''}
function doStart(){
  var p=E('authPwd').value;
  if(!p||p.length!==6){E('authMsg').innerText='Enter 6-digit password';return}
  addLog('Engine start...','info');
  fetch('/api/start_engine?pwd='+p).then(function(r){
    if(r.status===200||r.status===202){addLog('OK','ok');closeAuth();return}
    return r.text().then(function(t){if(r.status===401)E('authMsg').innerText='Wrong password';else E('authMsg').innerText=t})
  }).catch(function(){addLog('Network error','err')});
}
function doApi(url){addLog('Action: '+url,'info');fetch(url).then(function(r){return r.text()}).then(function(t){addLog('OK: '+t,'ok')}).catch(function(){addLog('Fail','err')});}
function openSettings(){
  E('settingsOverlay').style.display='flex';
  fetch('/api/status').then(function(r){return r.json()}).then(function(d){
    if(E('cfgSsid'))E('cfgSsid').value=d.wifi_ssid||'';
    if(E('cfgBt'))E('cfgBt').value=d.bt_name||'';
    if(E('cfgSec'))E('cfgSec').checked=d.sec_auth||false;
    if(E('cfgBleScan'))E('cfgBleScan').checked=d.ble_scan||false;
  }).catch(function(){});
}
function closeSettings(){E('settingsOverlay').style.display='none'}
function saveSettings(){
  var u='/api/update_config?new_ssid='+encodeURIComponent(E('cfgSsid').value)+'&new_bt='+encodeURIComponent(E('cfgBt').value);
  if(E('cfgPass').value)u+='&new_pass='+encodeURIComponent(E('cfgPass').value);
  if(E('cfgPwd').value)u+='&new_pwd='+encodeURIComponent(E('cfgPwd').value);
  u+='&sec_auth='+E('cfgSec').checked+'&ble_scan='+E('cfgBleScan').checked;
  fetch(u,{method:'POST'}).then(function(r){
    if(r.status===200){addLog('Saved','ok');closeSettings()}
    else{addLog('Save failed: '+r.status,'err')}
  }).catch(function(){addLog('Network error','err')});
}
function rebootDevice(){if(confirm('Restart?')){fetch('/api/reboot');setTimeout(function(){location.reload()},3000)}}
function updateStatus(){
  fetch('/api/status').then(function(r){return r.json()}).then(function(d){
    var rn=d.engine_running;
    E('engTitle').innerText=rn?'STOP':'START';
    E('engSub').innerText=rn?'Engine running':'Click to start';
    E('battery').innerText=d.voltage?d.voltage.toFixed(1)+' V':'--.- V';
  }).catch(function(){});
}
function updatePairStatus(){
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
}
function startPair(){
  addLog('Starting pairing...','info');
  fetch('/api/ble/pairing/start').then(function(r){return r.json()}).then(function(d){
    addLog(d.message,'ok');updatePairStatus();
  }).catch(function(){addLog('Fail','err')});
}
function stopPair(){
  fetch('/api/ble/pairing/stop').then(function(r){return r.json()}).then(function(d){
    addLog(d.message,'ok');updatePairStatus();
  }).catch(function(){addLog('Fail','err')});
}
function clearPair(){
  if(!confirm('Clear pairing?'))return;
  fetch('/api/ble/pairing/clear').then(function(r){return r.json()}).then(function(d){
    addLog(d.message,'ok');updatePairStatus();
  }).catch(function(){addLog('Fail','err')});
}
addLog('System ready','ok');
updateStatus();
updatePairStatus();
setInterval(updateStatus,2000);
setInterval(updatePairStatus,3000);
</script>
</body>
</html>"""

lines = html.split('\n')
output = ['#pragma once', '#include <pgmspace.h>', 'const char kEmbeddedIndexPage[] PROGMEM =']
for i, line in enumerate(lines):
    escaped = line.replace('\\', '\\\\').replace('"', '\\"')
    if i < len(lines) - 1:
        output.append('"' + escaped + '\\n"')
    else:
        output.append('"' + escaped + '\\n"')
output.append('"\\n"')
output.append(';')
output.append('')

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="utf-8", newline="") as f:
    f.write('\n'.join(output))
print(f"Generated {len(output)} lines, {len(html)} bytes")
