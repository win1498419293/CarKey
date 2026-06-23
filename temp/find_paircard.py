with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Find all occurrences of pairCard
import re
for m in re.finditer(r'pairCard', html):
    start = max(0, m.start()-50)
    end = min(len(html), m.end()+50)
    print(f"pairCard at {m.start()}: ...{html[start:end]}...")
    print()

# Also find pBadge occurrences
for m in re.finditer(r'pBadge', html):
    start = max(0, m.start()-80)
    end = min(len(html), m.end()+80)
    print(f"pBadge at {m.start()}: ...{html[start:end]}...")
    print()