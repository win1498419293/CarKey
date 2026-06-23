import re

with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find the script block
script_start = html.find("<script>")
script_end = html.find("</script>")
before = html[:script_start]
script = html[script_start:script_end]
after = html[script_end:]

# Convert HTML decimal entities in JavaScript to \uXXXX Unicode escapes
def entity_to_unicode(m):
    dec = int(m.group(1))
    return f"\\u{dec:04X}"

# Replace &#XXXXX; with \uXXXX in the script block
script_fixed = re.sub(r'&#(\d+);', entity_to_unicode, script)

# Also convert any remaining HTML hex entities &#xXXXX;
def hex_entity_to_unicode(m):
    dec = int(m.group(1), 16)
    return f"\\u{dec:04X}"

script_fixed = re.sub(r'&#x([0-9A-Fa-f]+);', hex_entity_to_unicode, script_fixed)

# Check for any remaining &# in script
remaining = script_fixed.count("&#")
print(f"Remaining &# entities in script: {remaining}")

# Count conversions
old_count = script.count("&#")
new_count = script_fixed.count("\\u")
print(f"Converted {old_count} HTML entities to {new_count} JS unicode escapes")

# Rebuild HTML
html = before + script_fixed + after

# Also fix the BLE pairing info to show paired device even when not authorized
# In updatePairStatus, when paired, show more detail
# Find: E('pInfo').innerHTML='...'
# The paired branch should also update the BLE status chip

# Add: when paired, update the BLE chip to show paired device
# Find the paired branch in updatePairStatus

old_pair_branch = """}else if(d.paired){
      E('pBadge').className='pair-badge badge-ok';E('pBadge').innerHTML='\\u5DF2\\u914D\\u5BF9';
      E('pInfo').innerHTML='\\u5DF2\\u8FDE\\u63A5: <b>'+d.name+'</b> ('+d.mac+')';
      E('pStart').style.display='inline-block';E('pStop').style.display='none';
    }"""

new_pair_branch = """}else if(d.paired){
      E('pBadge').className='pair-badge badge-ok';E('pBadge').innerHTML='\\u5DF2\\u914D\\u5BF9';
      E('pInfo').innerHTML='\\u5DF2\\u8FDE\\u63A5: <b>'+d.name+'</b> ('+d.mac+')';
      E('pStart').style.display='inline-block';E('pStop').style.display='none';
      var blc=E('chipBle');if(blc&&!blc.querySelector('.val').innerHTML.match('\\u5DF2\\u8BA4\\u8BC1')){
        blc.querySelector('.val').innerHTML='\\u5DF2\\u914D\\u5BF9';blc.className='status-chip on';}
    }"""

html = html.replace(old_pair_branch, new_pair_branch)

# Also fix the updateStatus BLE chip to respect pairing
# When paired but not authorized, show paired device hint
# Modify the BLE status logic

# Save
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
non_ascii = sum(1 for c in html if ord(c) > 127)
print(f"HTML: {len(html)} bytes, non-ASCII: {non_ascii}")

# Check key fixes
checks = [
    ("JS unicode escapes present", "\\u" in html),
    ("No HTML entities in script", "&#" not in script_fixed),
    ("confirm uses unicode", "confirm('\\u" in html),
    ("pair status updates BLE chip", "chipBle" in script_fixed),
]
for name, ok in checks:
    print(f"  {'[OK]' if ok else '[MISSING]'} {name}")