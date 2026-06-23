with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

print("=== Full file ===")
print(content[:300])
print("...")
print(content[-60:])

# Check: does the raw string capture newlines correctly?
idx = content.find("R\"HTMLEND(")
end = content.find(")HTMLEND\"")
print(f"\nRaw string body ({end - idx - 12} chars)")
body = content[idx+12:end]
print(f"Body starts with: {repr(body[:80])}")
print(f"Body ends with: {repr(body[-40:])}")