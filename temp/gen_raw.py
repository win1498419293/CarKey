with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Verify the HTML is ASCII
non_ascii = sum(1 for c in html if ord(c) > 127)
print(f"HTML: {len(html)} bytes, non-ASCII: {non_ascii}")

# Generate using C++11 raw string literal
# We need a delimiter that doesn't appear in the HTML
delimiter = "HTMLEND"
if delimiter in html:
    print(f"WARNING: delimiter {delimiter} appears in HTML!")
else:
    print(f"Delimiter {delimiter} is safe")

# Check for )\" which would break raw string
if ')\"' in html:
    print("WARNING: )\" found in HTML - raw string might break")

output = f'''#pragma once

const char kEmbeddedIndexPage[] = R"{delimiter}(
{html}
){delimiter}";
'''

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="ascii", newline="") as f:
    f.write(output)

print(f"Generated raw string literal, {len(output)} bytes")

# Verify output
with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "rb") as f:
    raw = f.read()
non_ascii2 = sum(1 for b in raw if b > 127)
print(f"Output non-ASCII bytes: {non_ascii2}")