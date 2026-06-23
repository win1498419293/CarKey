with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

# Add PROGMEM back
content = content.replace(
    "const char kEmbeddedIndexPage[] = R\"HTMLEND(",
    "const char kEmbeddedIndexPage[] PROGMEM = R\"HTMLEND("
)

# Also add the pgmspace include back (needed for PROGMEM on some platforms)
content = content.replace(
    "#pragma once\n\n",
    "#pragma once\n#include <pgmspace.h>\n\n"
)

with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "w", encoding="ascii", newline="") as f:
    f.write(content)

print("PROGMEM added back")
print(f"First line: {content[:70]}")