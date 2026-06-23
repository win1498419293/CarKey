with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Fix: insert space before "> in )"> patterns (not inside strings, just in HTML tags)
# Replace )"> with )" > to avoid raw string termination )" 
# But we need to be careful: only in HTML context, not in JS strings

# In the HTML, patterns like: onclick="foo()">  become onclick="foo()" >
# This is valid HTML
html = html.replace(')" onclick', ')" onclick')  # no change needed here actually

# The actual issue: the raw string delimiter is )HTMLEND"
# If the content has )" anywhere, it terminates early
# Let me use a more unique delimiter

# Actually, the simplest fix: change )" > in HTML tags to ) " >
# Pattern: )">  (from HTML attributes)
import re
# Match )"> that appears in HTML context (after onclick, etc.)
# We'll replace )" with ) " (adding a space) wherever it appears in HTML tags
html = html.replace(')">', ')" >')
# Also handle )" followed by space then > (already have space)
# Also handle )"style= -> )" style=
html = html.replace(')" style=', ')" style=')  # already correct?

# Check remaining )" occurrences
remaining = html.count(')"')
print(f"Remaining )\" occurrences: {remaining}")
if remaining > 0:
    for m in re.finditer(r'\)"', html):
        start = max(0, m.start()-10)
        end = min(len(html), m.end()+15)
        print(f"  pos {m.start()}: ...{html[start:end]}...")

# Save updated HTML
with open(r"D:\CarKey_V5\temp\page.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)
print("Updated page.html saved")