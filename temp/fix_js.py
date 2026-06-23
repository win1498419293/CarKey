with open(r"D:\CarKey_V5\data\index.html", "r", encoding="utf-8") as f:
    html = f.read()

# Remove duplicate JS blocks - find second occurrence of pairing functions
first = html.find("function updatePairStatus()")
second = html.find("function updatePairStatus()", first + 10)

if second > 0:
    sys_ready = html.find("addLog('System ready', 'ok');", second)
    if sys_ready > 0:
        after = html.find("\n", sys_ready)
        html = html[:second] + html[after:]
        print("Removed duplicate JS block")

# Remove duplicate setInterval
html = html.replace(
    "setInterval(updatePairStatus, 3000);\nsetInterval(updatePairStatus, 3000);",
    "setInterval(updatePairStatus, 3000);"
)

with open(r"D:\CarKey_V5\data\index.html", "w", encoding="utf-8", newline="") as f:
    f.write(html)
print(f"Fixed, size: {len(html)} bytes")
