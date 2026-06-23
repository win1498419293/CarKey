with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Change send back to send_P for PROGMEM data
old = 'server.send(200, "text/html; charset=utf-8", kEmbeddedIndexPage)'
new = 'server.send_P(200, "text/html; charset=utf-8", kEmbeddedIndexPage)'

count = content.count(old)
content = content.replace(old, new)

with open(r"D:\CarKey_V5\src\WebManager.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print(f"Restored send_P in {count} places")