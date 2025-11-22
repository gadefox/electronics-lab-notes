path = "flash.bin"

with open(path, "rb+") as f:
  data = f.read()
  i = len(data) - 1
  while i >= 0 and data[i] == 0xFF:
    i -= 1
  if i < len(data) - 1:
    f.seek(i + 1)
    f.truncate()
    print(f"Trimmed {len(data) - i - 1} trailing FF bytes")
  else:
    print("No trailing FF to trim")
