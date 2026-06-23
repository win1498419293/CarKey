import re

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

# Reconstruct HTML
pattern = re.compile(r'^"(.+?)\\n"$')
reconstructed = []
for line in content.split("\n"):
    m = pattern.match(line)
    if m:
        reconstructed.append(m.group(1))

html = "\n".join(reconstructed)

# Check a specific section - the status bar
idx = html.find("status-bar")
if idx >= 0:
    snippet = html[idx:idx+300]
    print("Status bar section:")
    print(snippet)
    print()

# Check the auth overlay label
idx2 = html.find("authPwd")
if idx2 >= 0:
    snippet2 = html[idx2-60:idx2+80]
    print("Auth password section:")
    print(snippet2)
    print()

# Check for literal &# in places that should render
literal_entities = re.findall(r'&#\d+;', html)
print(f"Total HTML entities to render: {len(literal_entities)}")

# Check for C escape sequences that might not decode right
if '\\"' in html:
    print("\nWARNING: literal backslash-quote in reconstructed HTML!")
if '\\n' in html:
    print("WARNING: literal backslash-n in reconstructed HTML!")

# Check a script string with Chinese
idx3 = html.find("authMsg")
if idx3 >= 0:
    snippet3 = html[idx3:idx3+150]
    print("\nAuth message section:")
    print(snippet3)