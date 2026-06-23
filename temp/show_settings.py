with open(r"D:\CarKey_V5\temp\page.html", "r", encoding="utf-8") as f:
    html = f.read()

# Show settings overlay
settings_start = html.find("settingsOverlay")
script_start = html.find("<script>")
if settings_start > 0 and script_start > settings_start:
    settings_html = html[settings_start:script_start]
    print(settings_html[:2000])