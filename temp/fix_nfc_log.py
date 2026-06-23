with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Add _lastLocked tracking alongside _lastEngine
# Find: var _lastEngine=null;
# Add after it: var _lastLocked=null;
old = "var _lastEngine=null;"
new = "var _lastEngine=null;var _lastLocked=null;"
html = html.replace(old, new)

# In updateStatus, after _lastEngine tracking, add _lastLocked tracking
# Find the line after _lastEngine code and before var btn=E('engBtn')
old2 = "var btn=E('engBtn');"
new2 = "var cl=d.config_locked;\n    if(_lastLocked!==null&&_lastLocked!==cl){addLog(cl?'NFC&#24050;&#38145;&#23450;&#37197;&#32622;':'NFC&#24050;&#35299;&#38145;',cl?'warn':'ok')}\n    _lastLocked=cl;\n    var btn=E('engBtn');"
html = html.replace(old2, new2)

# Also fix: now the _lastEngine variable is declared twice (once globally, once in the old body)
# Let me check and fix the duplicate
# The original updateStatus had _lastEngine declared before the function
# And inside updateStatus, the line: var rn=d.engine_running; was replaced with tracking code
# But we also have _lastEngine declared before the function, which is correct

# Remove any duplicate _lastEngine declaration inside the function body
# Find: _lastEngine=rn;\n    _lastEngine=rn;
html = html.replace("_lastEngine=rn;\n    _lastEngine=rn;", "_lastEngine=rn;")

with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
checks = [
    ("_lastLocked", "_lastLocked" in html),
    ("NFC log message", "NFC&#24050;&#38145;&#23450;" in html or "NFC已锁定" in html),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")

non_ascii = sum(1 for c in html if ord(c) > 127)
print(f"Non-ASCII: {non_ascii}")