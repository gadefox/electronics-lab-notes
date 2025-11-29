import re

total = 0
pat = re.compile(r'^\s*([0-9A-Fa-f]{6}):\s*((?:[0-9A-Fa-f]{2}\s+){1,16})')

inp = 'flash.hex'
with open(inp,'r',errors='ignore') as f, open(out,'wb') as g:
  for line in f:
    m = pat.match(line)
    if m:
      hexbytes = m.group(2).strip().split()
      g.write(bytes(int(x,16) for x in hexbytes))
      total += len(hexbytes)
print("Wrote", total, "bytes to", out)

out = 'flash.bin'
with open(out,'rb') as g:
  g.seek(0,2)
  size = g.tell()
  print("Size:", size, "bytes (expected 4194304 for 4MB)")
  for n in (16,64):
    g.seek(max(0,size-n))
    tail = g.read(n)
    print(f"Last {n} bytes:", tail.hex(' ').upper())
