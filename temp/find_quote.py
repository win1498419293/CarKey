with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find all occurrences of )"
import re
for m in re.finditer(r'\)"', html):
    start = max(0, m.start()-20)
    end = min(len(html), m.end()+20)
    print(f"Position {m.start()}: ...{html[start:end]}...")