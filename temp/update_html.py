import re

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="utf-8") as f:
    lines = f.readlines()

# Collect PROGMEM lines
html_lines = []
preamble = []
state = "preamble"
for line in lines:
    if state == "preamble":
        preamble.append(line.rstrip('\n'))
        if 'const char kEmbeddedIndexPage[] PROGMEM =' in line:
            state = "html"
        continue
    if state == "html":
        stripped = line.strip()
        if stripped == ';':
            state = "done"
            continue
        if stripped == '"\n"' or stripped == '"\\n"':
            # extra blank line in PROGMEM, skip
            continue
        # Extract: "content\n"
        if stripped.startswith('"') and len(stripped) > 2:
            # Remove leading "
            inner = stripped[1:]
            # Remove trailing \n" or \n";  (C escape for newline then close-quote)
            if inner.endswith('\\n"'):
                inner = inner[:-3]
            elif inner.endswith('\\n";'):
                inner = inner[:-4]
            else:
                # might be last line without \n
                if inner.endswith('"'):
                    inner = inner[:-1]
                elif inner.endswith('";'):
                    inner = inner[:-2]
            # Unescape C string: \\ -> \, \" -> "
            inner = inner.replace('\\\\', '\\').replace('\\"', '"')
            html_lines.append(inner)
        continue

html = '\n'.join(html_lines)
print(f"Extracted HTML: {len(html)} chars, {len(html_lines)} lines")

# --- Apply modifications ---

# 1. CSS for pairing (before </style>)
css = """
.pair-card { background: var(--card); border-radius: 12px; padding: 12px 14px; margin-bottom: 14px; border: 1px solid var(--border); }
.pair-header { display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; }
.pair-title { font-size: 11px; color: var(--blue); letter-spacing: 1px; display: flex; align-items: center; gap: 6px; }
.pair-status { font-size: 10px; padding: 3px 8px; border-radius: 10px; }
.pair-status-paired { background: rgba(16,185,129,0.2); color: var(--green); }
.pair-status-none { background: rgba(100,116,139,0.2); color: var(--sub); }
.pair-status-pairing { background: rgba(59,130,246,0.2); color: var(--blue); animation: pulse 1.5s infinite; }
@keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.5} }
.pair-info { font-size: 10px; color: var(--sub); margin: 4px 0; }
.pair-info span { color: var(--txt); }
.pair-btn-row { display: flex; gap: 6px; margin-top: 8px; }
.pair-btn { font-size: 10px; padding: 6px 12px; border-radius: 6px; border: none; cursor: pointer; transition: 0.2s; }
.pair-btn-start { background: var(--blue); color: #fff; }
.pair-btn-start:disabled { opacity: 0.4; cursor: not-allowed; }
.pair-btn-stop { background: var(--red); color: #fff; }
.pair-btn-ghost { background: transparent; border: 1px solid var(--border); color: var(--sub); }
"""

if "</style>" in html:
    html = html.replace("</style>", css + "</style>", 1)
    print("CSS added")
else:
    print("WARNING: </style> not found")

# 2. Add pairing card
pair_card = """<div class="pair-card" id="pairCard">
  <div class="pair-header">
    <div class="pair-title"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6.5 6.5l11 11"/></svg> Bluetooth</div>
    <div class="pair-status pair-status-none" id="pairStatusBadge">Not Paired</div>
  </div>
  <div class="pair-info" id="pairInfo">Tap Start Pairing to begin</div>
  <div class="pair-btn-row">
    <button class="pair-btn pair-btn-start" id="btnPairStart" onclick="startBlePairing()">Start Pairing</button>
    <button class="pair-btn pair-btn-stop" id="btnPairStop" onclick="stopBlePairing()" style="display:none;">Stop</button>
    <button class="pair-btn pair-btn-ghost" id="btnPairClear" onclick="clearBlePairing()">Clear</button>
  </div>
</div>
"""

old_meta = '<div class="device-meta" id="devMeta">Loading...</div>'
if old_meta in html:
    html = html.replace(old_meta, pair_card + "\n" + old_meta, 1)
    print("Pair card added")
else:
    print("WARNING: device-meta not found")

# 3. Add JS functions
pair_js = """function updatePairStatus() {
  fetch('/api/ble/pairing/status').then(function(r) { return r.json(); }).then(function(d) {
    var badge = document.getElementById('pairStatusBadge');
    var info = document.getElementById('pairInfo');
    var btnStart = document.getElementById('btnPairStart');
    var btnStop = document.getElementById('btnPairStop');
    if (d.pairing) {
      if (badge) { badge.className = 'pair-status pair-status-pairing'; badge.innerText = 'Pairing...'; }
      if (info) info.innerText = 'Find and connect in phone Bluetooth settings';
      if (btnStart) btnStart.style.display = 'none';
      if (btnStop) btnStop.style.display = 'inline-block';
    } else if (d.paired) {
      if (badge) { badge.className = 'pair-status pair-status-paired'; badge.innerText = 'Paired'; }
      var devInfo = (d.name && d.name.length > 0) ? d.name : 'Unknown';
      if (d.mac && d.mac.length > 0) devInfo += ' (' + d.mac + ')';
      if (info) info.innerHTML = 'Paired: <span>' + devInfo + '</span>';
      if (btnStart) btnStart.style.display = 'inline-block';
      if (btnStop) btnStop.style.display = 'none';
    } else {
      if (badge) { badge.className = 'pair-status pair-status-none'; badge.innerText = 'Not Paired'; }
      if (info) info.innerText = 'Tap Start Pairing to begin';
      if (btnStart) btnStart.style.display = 'inline-block';
      if (btnStop) btnStop.style.display = 'none';
    }
  }).catch(function() {});
}

function startBlePairing() {
  addLog('Starting Bluetooth pairing...', 'info');
  fetch('/api/ble/pairing/start').then(function(r) { return r.json(); }).then(function(d) {
    addLog(d.message, 'ok');
    updatePairStatus();
  }).catch(function() { addLog('Pairing start failed', 'err'); });
}

function stopBlePairing() {
  fetch('/api/ble/pairing/stop').then(function(r) { return r.json(); }).then(function(d) {
    addLog(d.message, 'ok');
    updatePairStatus();
  }).catch(function() { addLog('Pairing stop failed', 'err'); });
}

function clearBlePairing() {
  if (!confirm('Clear Bluetooth pairing?')) return;
  fetch('/api/ble/pairing/clear').then(function(r) { return r.json(); }).then(function(d) {
    addLog(d.message, 'ok');
    updatePairStatus();
  }).catch(function() { addLog('Clear failed', 'err'); });
}
"""

old_addlog = "addLog('System ready', 'ok');"
if old_addlog in html:
    html = html.replace(old_addlog, pair_js + "\n" + old_addlog, 1)
    print("JS added")
else:
    print("WARNING: addLog not found")

# 4. Add polling
html = html.replace("setInterval(updateStatus, 2000);",
                    "setInterval(updateStatus, 2000);\nsetInterval(updatePairStatus, 3000);", 1)
html = html.replace("updateStatus();\nrefreshEngine();",
                    "updateStatus();\nupdatePairStatus();\nrefreshEngine();", 1)
print("Polling added")

# --- Re-encode to PROGMEM format ---
output = []
output.extend(preamble)

for line in html.split('\n'):
    # Escape for C string literal: \ -> \\, " -> \"
    escaped = line.replace('\\', '\\\\').replace('"', '\\"')
    output.append('"' + escaped + '\\n"')

# Add trailing blank line and semicolon (matching original format)
output.append('"\n"')
output.append(';')
output.append('')

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="utf-8", newline='\r\n') as f:
    f.write('\r\n'.join(output))

print(f"Written {len(output)} lines")
