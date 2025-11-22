import sys, zlib
from pathlib import Path

if len(sys.argv) < 2:
  print(f"Usage: {sys.argv[0]} file")
  sys.exit(1)

path = Path(sys.argv[1])
if not path.exists():
  print(f"Error: file '{sys.argv[1]}' not found")
  sys.exit(1)
 
data = open(sys.argv[1], "rb").read()
co = zlib.compressobj(level=9, wbits=-15)   # -15 => raw deflate
out = co.compress(data) + co.flush()

open(sys.argv[1] + ".zlip", "wb").write(out)
print("wrote deflate, len:", len(out))
