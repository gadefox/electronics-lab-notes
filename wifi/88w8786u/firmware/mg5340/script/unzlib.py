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
out = zlib.decompress(data)

open(sys.argv[1] + ".decomp", "wb").write(out)
