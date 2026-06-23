with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

non_ascii = sum(1 for c in html if ord(c) > 127)
print(f"HTML: {len(html)} bytes, non-ASCII: {non_ascii}")

# Verify no )HTMLEND" in content
if ")HTMLEND\"" in html:
    print("FATAL: delimiter found in HTML!")
else:
    print("Delimiter safe")

output = '#pragma once\n\nconst char kEmbeddedIndexPage[] = R"HTMLEND(\n' + html + '\n)HTMLEND";\n'

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="ascii", newline="") as f:
    f.write(output)

print(f"Generated raw string literal, {len(output)} bytes")

# Verify
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "rb") as f:
    raw = f.read()
print(f"Non-ASCII bytes: {sum(1 for b in raw if b > 127)}")
print(f"First line: {raw[:50].decode('ascii')}")
print(f"Last 30 bytes: {raw[-30:].decode('ascii', errors='replace')}")