def crc32(data: bytes) -> int:
  crc = 0
  for byte in data:
    crc ^= byte << 24
    for _ in range(8):
      if crc & 0x80000000:
        crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
      else:
        crc = (crc << 1) & 0xFFFFFFFF
  return crc

with open("blob.bin", "rb") as file:
  data = file.read()

  i = 0
  while i < len(data) - 16:
    cmd = data[i]
    if cmd in {1, 4, 5, 6, 7, 10, 21} and data[i+1:i+4] == b'\x00\x00\x00':
      header = data[i:i+16]
      if crc32(header) == 0:
        print(f"{i}")
        break
      i += 4
    else:
      i += 1
