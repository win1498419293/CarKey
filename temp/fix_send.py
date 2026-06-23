with open(r"D:\CarKey_V5\src\WebManager.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Replace send_P with send for the HTML page
old = 'server.send_P(200, "text/html; charset=utf-8", kEmbeddedIndexPage)'
new = 'server.send(200, "text/html; charset=utf-8", kEmbeddedIndexPage)'

count = content.count(old)
content = content.replace(old, new)

with open(r"D:\CarKey_V5\src\WebManager.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print(f"Replaced {count} occurrences of send_P -> send")