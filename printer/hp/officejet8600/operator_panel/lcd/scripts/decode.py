import re, sys, bisect
from PIL import Image

src = sys.argv[1] if len(sys.argv) > 1 else 'header.hex'
out = sys.argv[2] if len(sys.argv) > 2 else 'decoded_text.png'
PORCH, DATA, LINE = 35, 480, 523

ch = {'CLK': [], 'DATA': [], 'HSYNC': []}
for l in open(src):
    m = re.match(r'(\w+):([0-9a-f ]+)', l.strip())
    if m and m.group(1) in ch:
        for b in m.group(2).split():
            v = int(b, 16)
            ch[m.group(1)] += [(v >> (7 - i)) & 1 for i in range(8)]
c, d, h = ch['CLK'], ch['DATA'], ch['HSYNC']
n = len(c)

starts, s = [], None
for i in range(n):
    if h[i] == 0 and s is None: s = i
    if h[i] == 1 and s is not None: starts.append(s); s = None

edges_all = [i for i in range(1, n) if c[i] and not c[i-1]]

rows = []
for s in starts:
    i = bisect.bisect_left(edges_all, s)
    e = edges_all[i:i+LINE]
    if len(e) == LINE:
        rows.append([d[x] for x in e])
print(len(rows), 'lines')

raw = Image.new('L', (DATA, len(rows)))
for y, b in enumerate(rows):
    for x in range(DATA):
        raw.putpixel((x, y), 255 if b[PORCH+x] else 0)

px = Image.new('L', (320, len(rows)))
for y, b in enumerate(rows):
    for x in range(320):
        px.putpixel((x, y), 255 if b[PORCH + min(DATA-1, int(x*1.5+0.75))] else 0)

crop = px.crop((0, 0, 160, len(rows)))
crop.resize((160*8, len(rows)*8), Image.NEAREST).save(out)
raw.save('raw480.png')
for b in rows:
    print(''.join('#' if v else '.' for v in b[PORCH:PORCH+DATA][:200]))
