with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Fix: change .innerText to .innerHTML in JavaScript (so HTML entities get decoded)
# But be careful: only in <script> context, and not for .innerText = '' (clearing)

# Strategy: in the <script> block, replace .innerText= with .innerHTML=
# for assignments that contain &# (entity strings)
import re

# Find the script block
script_start = html.find("<script>")
script_end = html.find("</script>")
if script_start >= 0 and script_end >= 0:
    before = html[:script_start]
    script = html[script_start:script_end]
    after = html[script_end:]
    
    # Replace .innerText with .innerHTML (safe - empty string assignment also works with innerHTML)
    script = script.replace('.innerText', '.innerHTML')
    
    html = before + script + after
    print("Replaced .innerText -> .innerHTML in script block")
else:
    print("WARNING: script block not found!")

# Also check if there's .innerText outside script (there shouldn't be)
remaining = html.count('.innerText')
print(f"Remaining .innerText: {remaining}")

# Save updated
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)

# Verify
non_ascii = sum(1 for c in html if ord(c) > 127)
print(f"Updated HTML: {len(html)} bytes, non-ASCII: {non_ascii}")