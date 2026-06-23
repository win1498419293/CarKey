line = '</html>'
escaped = line.replace('\\', '\\\\').replace('"', '\\"')
result = '"' + escaped + '\\n"'
print('Result:', repr(result))
print('Bytes:', result.encode('utf-8'))

# Verify by round-trip
with open(r'D:\CarKey_V5\temp\test_out.txt', 'w') as f:
    f.write(result + '\n')
