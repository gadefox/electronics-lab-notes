import sys
from colorama import init, Fore, Style

init(autoreset=True)

def parse(path):
  samples = []

  with open(path) as f:
    for lineno, line in enumerate(f, 1):
      s = line.strip()
      if not s:
        continue

      p = s.split(' ')
      if len(p) == 2:
        samples.append((lineno, int(p[0]), int(p[1])))

  return samples

def extract(samples):
  events = []
  val, bits = 0, 0
  lineno, sclp, sdap = samples[0]

  for lineno, scl, sda in samples[1:]:
    if scl:
      if sclp:                              # SCL high -> SDA change
        if not sda and sdap:
          events.append((lineno, 'S'))
          val, bits = 0, 0
        elif sda and not sdap:
          events.append((lineno, 'P'))
          val, bits = 0, 0
      else:                                 # SCL rising edge -> bit
        if bits == 8:
          events.append((lineno, 'B', val, sda))
          val, bits = 0, 0
        else:
          val = (val << 1) | sda
          bits += 1

    sclp, sdap = scl, sda

  return events

def decode(events):
  trans, data = [], []
  lineno, val, ack = -1, 0, 0

  for e in events:
    if e[1] == 'S':
      if lineno != -1:
        trans.append((lineno, val, ack, data))
        val, ack, data = 0, 0, []

      lineno = e[0]
    elif e[1] == 'B':
      if val:
        data.append((e[2], e[3]))
      else:
        val, ack = e[2], e[3]

  return trans

def data2hex(data):
  hex = []
  prev = 0

  for val, ack in data:
    if prev == 0x80:
      hex.append(Fore.YELLOW + f"{val:02X}" + Style.RESET_ALL)
      prev = 0
    elif prev == 0x40:
      hex.append(Fore.BLUE + f"{val:02X}" + Style.RESET_ALL)
    else:
      hex.append(f"{val:02X}")
      prev = val

  return hex

def filter(trans, addr):
  for t in trans:
    if t[1] != addr:
      continue

    addr = t[1] >> 1
    rw = '→' if t[1] & 1 else '←'

    hex = data2hex(t[3])
    size = len(hex)

    print(f"[{t[0]}] " + Fore.RED + f"{addr:02X} " + Style.RESET_ALL +
          f"{rw} {size}" + " bytes:")

    for i in range(0, size, 28):
      print('  ' + ' '.join(hex[i:i+28]))

if __name__ == '__main__':
  path = sys.argv[1] if len(sys.argv) > 1 else 'lcd.log'
  samples = parse(path)
  events = extract(samples)
  trans = decode(events)
  filter(trans, 0x3C)
