with open(r"D:\CarKey_V5\data\index.html", "r", encoding="utf-8") as f:
    html = f.read()

# Remove duplicate updatePairStatus call (keep only first after updateStatus)
# Replace "updatePairStatus();\nupdatePairStatus();" with "updatePairStatus();"
html = html.replace("updatePairStatus();\nupdatePairStatus();", "updatePairStatus();")

# Also fix: "setInterval(updateStatus, 2000);\n..." might have extra pairing call
# Remove any standalone duplicate updatePairStatus() calls right before refreshEngine
html = html.replace("updatePairStatus();\nrefreshEngine();", "refreshEngine();")

with open(r"D:\CarKey_V5\data\index.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)
print(f"Size: {len(html)} bytes")

# Verify no more duplicates
count = html.count("function updatePairStatus")
print(f"updatePairStatus definitions: {count}")
count2 = html.count("setInterval(updatePairStatus")
print(f"setInterval(updatePairStatus) calls: {count2}")
