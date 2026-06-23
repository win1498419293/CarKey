with open(r"D:\CarKey_V5\src\Config.cpp", "r", encoding="utf-8") as f:
    cpp = f.read()

func_start = cpp.find("bool saveSystemConfig()")
func_end = cpp.find("\nbool nfcUnlockEnabled", func_start)
func = cpp[func_start:func_end]

bare_returns = func.count("return;")
false_returns = func.count("return false;")
ok_returns = func.count("return ok;")

print(f"Bare 'return;' in saveSystemConfig: {bare_returns}")
print(f"'return false;' in saveSystemConfig: {false_returns}")
print(f"'return ok;' in saveSystemConfig: {ok_returns}")
print("COMPILE OK" if bare_returns == 0 else "STILL HAS BARE RETURN!")