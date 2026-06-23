with open(r"D:\CarKey_V5\include\EmbeddedIndexPage.h", "r", encoding="ascii") as f:
    content = f.read()

# Show first 150 chars
print("=== FIRST 150 CHARS ===")
print(repr(content[:150]))

# Show last 80 chars
print("\n=== LAST 80 CHARS ===")
print(repr(content[-80:]))

# Verify the raw string is properly formed
idx = content.find("R\"HTMLEND(")
print(f"\nRaw string starts at position: {idx}")
end_idx = content.find(")HTMLEND\"")
print(f"Raw string ends at position: {end_idx}")