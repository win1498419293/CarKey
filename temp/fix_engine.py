with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# ===== FIX 1: Engine button - toggle start/stop =====
# Change onclick from openAuth() to toggleEngine()
old_onclick = 'onclick="openAuth()"'
new_onclick = 'onclick="toggleEngine()"'
html = html.replace(old_onclick, new_onclick)

# ===== FIX 2: Auth overlay - support both start and stop modes =====
# Change the auth modal title dynamically via JS, keep overlay structure same

# ===== FIX 3: Add toggleEngine() and modify doStart() to handle stop =====
# Find the script block
script_start = html.find("<script>")
script_end = html.find("</script>")
script = html[script_start:script_end]

# Add toggleEngine function and modify openAuth/doStart
old_openAuth = "function openAuth(){E('authOverlay').style.display='flex';E('authPwd').focus()}"
new_openAuth = "function openAuth(m){window._authMode=m||'start';E('authOverlay').style.display='flex';E('authPwd').focus();var t=m==='stop'?'&#20572;&#27490;&#39564;&#35777;':'&#21551;&#21160;&#39564;&#35777;';E('authModalTitle').innerHTML=t;var b=m==='stop'?'&#20572;&#27490;&#24341;&#25806;':'&#21551;&#21160;&#24341;&#25806;';E('authSubmitBtn').innerHTML=b}"
script = script.replace(old_openAuth, new_openAuth)

# Add toggleEngine function right after openAuth
toggle_func = "function toggleEngine(){var rn=E('engBtn').classList.contains('running');if(rn){if(confirm('&#30830;&#23450;&#20572;&#27490;&#24341;&#25806;?')){openAuth('stop')}}else{openAuth('start')}}"
script = script.replace("function openAuth(m)", toggle_func + "\nfunction openAuth(m)")

# Modify doStart to handle stop mode
old_doStart = """function doStart(){
  var p=E('authPwd').value;
  if(!p||p.length!==6){E('authMsg').innerHTML='&#35831;&#36755;&#20837;6&#20301;&#23494;&#30721;';return}
  addLog('&#24341;&#25806;&#21551;&#21160;...','info');
  fetch('/api/start_engine?pwd='+p).then(function(r){
    if(r.status===200||r.status===202){addLog('&#21551;&#21160;&#25104;&#21151;','ok');closeAuth();return}
    return r.text().then(function(t){if(r.status===401)E('authMsg').innerHTML='&#23494;&#30721;&#38169;&#35823;';else E('authMsg').innerHTML=t})
  }).catch(function(){addLog('&#32593;&#32476;&#38169;&#35823;','err')});
}"""

new_doStart = """function doStart(){
  var p=E('authPwd').value;
  if(!p||p.length!==6){E('authMsg').innerHTML='&#35831;&#36755;&#20837;6&#20301;&#23494;&#30721;';return}
  var mode=window._authMode||'start';
  var url=mode==='stop'?'/api/stop_engine?pwd='+p:'/api/start_engine?pwd='+p;
  var label=mode==='stop'?'&#20572;&#27490;&#24341;&#25806;':'&#21551;&#21160;&#24341;&#25806;';
  addLog(label+'...','info');
  fetch(url).then(function(r){
    if(r.status===200||r.status===202){addLog(label+'&#25104;&#21151;','ok');closeAuth();return}
    return r.text().then(function(t){if(r.status===401)E('authMsg').innerHTML='&#23494;&#30721;&#38169;&#35823;';else if(r.status===409)E('authMsg').innerHTML='&#24341;&#25806;&#26410;&#36816;&#34892;';else E('authMsg').innerHTML=t})
  }).catch(function(){addLog('&#32593;&#32476;&#38169;&#35823;','err')});
}"""
script = script.replace(old_doStart, new_doStart)

# Add auth modal elements IDs for dynamic title/button
# Find the auth overlay div and add IDs
old_auth_title = '<div class="modal-title">&#128272; &#21551;&#21160;&#39564;&#35777;</div>'
new_auth_title = '<div class="modal-title" id="authModalTitle">&#128272; &#21551;&#21160;&#39564;&#35777;</div>'
script = script.replace(old_auth_title, new_auth_title)

old_auth_btn = '<button class="btn btn-primary" onclick="doStart()">&#21551;&#21160;&#24341;&#25806;</button>'
new_auth_btn = '<button class="btn btn-primary" id="authSubmitBtn" onclick="doStart()">&#21551;&#21160;&#24341;&#25806;</button>'
script = script.replace(old_auth_btn, new_auth_btn)

# ===== FIX 4: Add system log polling for status changes =====  
# After updateStatus(), log status changes
# Find updateStatus and add change detection
old_update_status_end = "}).catch(function(){});}"
new_update_status_end = """).catch(function(){});}
var _lastEngine=null;
function updateStatus(){
  fetch('/api/status').then(function(r){return r.json()}).then(function(d){
    var rn=d.engine_running;
    if(_lastEngine!==null&&_lastEngine!==rn){addLog(rn?'&#24341;&#25806;&#24050;&#21551;&#21160;':'&#24341;&#25806;&#24050;&#20572;&#27490;',rn?'ok':'info')}
    _lastEngine=rn;"""

# Replace the updateStatus function
old_update_start = "function updateStatus(){\n  fetch('/api/status')"
new_update_start = "function updateStatus(){\n  fetch('/api/status')"
script = script.replace(old_update_start, new_update_start)

# The updateStatus function body - add change tracking
old_body_line = "var rn=d.engine_running;"
new_body_line = "var rn=d.engine_running;\n    if(_lastEngine!==null&&_lastEngine!==rn){addLog(rn?'&#24341;&#25806;&#24050;&#21551;&#21160;':'&#24341;&#25806;&#24050;&#20572;&#27490;',rn?'ok':'info')}\n    _lastEngine=rn;"
script = script.replace(old_body_line, new_body_line)

# Add _lastEngine declaration before the function
script = script.replace("function updateStatus(){", "var _lastEngine=null;\nfunction updateStatus(){")

# Put the script back
html = html[:script_start] + script + html[script_end:]

# Also fix the auth overlay in the HTML body (not in script)
# The auth modal title and button are in the HTML body, let me fix those too
html = html.replace(
    '<div class="modal-title">&#128272; &#21551;&#21160;&#39564;&#35777;</div>',
    '<div class="modal-title" id="authModalTitle">&#128272; &#21551;&#21160;&#39564;&#35777;</div>'
)
html = html.replace(
    '<button class="btn btn-primary" onclick="doStart()">&#21551;&#21160;&#24341;&#25806;</button>',
    '<button class="btn btn-primary" id="authSubmitBtn" onclick="doStart()">&#21551;&#21160;&#24341;&#25806;</button>'
)

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

print("Updated page.html")

# Verify key changes
checks = [
    ("toggleEngine function", "function toggleEngine()" in html),
    ("openAuth with mode param", "openAuth(m)" in html),
    ("stop_engine API", "/api/stop_engine" in html),
    ("authModalTitle id", 'authModalTitle' in html),
    ("authSubmitBtn id", 'authSubmitBtn' in html),
    ("_lastEngine tracking", '_lastEngine' in html),
    ("engine onclick toggleEngine", 'toggleEngine()' in html.replace('<script>','').split('</script>')[0]),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")