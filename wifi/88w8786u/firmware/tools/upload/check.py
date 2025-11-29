import struct, sys
from pathlib import Path
from colorama import init, Fore, Style

init(autoreset=True)

def info(msg: str):
  print(Fore.CYAN + " " + msg)

def warn(msg: str):
  print(Fore.YELLOW + " " + msg)

def error(msg: str):
  print(Fore.RED + "❌" + msg)

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

def main():
  if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} firmware")
    return

  path = Path(sys.argv[1])
  if not path.exists():
    error(f"File '{sys.argv[1]}' not found")
    return

  seqnum = 0
  with open(path, "rb") as file:
    while True:
      header = file.read(16)  # 4x DWORD = 16 bytes
      length = len(header)
      if length == 0:
        break
      if length < 16:
        warn(f"Invalid packet ({length} bytes)")
        break

      crc = crc32(header)
      if crc != 0:
        warn(f"Header: invalid CRC (0x{crc:08X})")
        break

      seqnum += 1
      cmd, addr, length, crc = struct.unpack("<4I", header)
      if cmd == 4:
        if addr != 0:
          warn(f"Invalid addr value: {addr}")
        if length != 0:
          warn(f"Invalid length value: {length}")
        if crc != 411884319:
          warn(f"Invalid crc value: {crc}")
        print(Fore.BLUE + f"{seqnum} EXEC")
        continue
      if cmd == 6:
        if length != 0:
          warn(f"Invalid length value: {length}")
        print(Fore.BLUE + f"{seqnum} CMD6" + Style.RESET_ALL +
              ": data=" + Fore.YELLOW + f"0x{addr:08X}({addr})" + Style.RESET_ALL +
              " crc=" + Fore.YELLOW + f"0x{crc:08X}")
        continue
      if cmd == 7:
        if addr != length:
          warn(f"Invalid addr ({addr}) and length ({length}) values")
        print(Fore.BLUE + f"{seqnum} CMD7" + Style.RESET_ALL +
              ": data=" + Fore.YELLOW + f"{addr}" + Style.RESET_ALL +
              " crc=" + f"0x{crc:08X}")
        continue
      if cmd == 1:
        print(Fore.BLUE + f"{seqnum} DNLD" + Style.RESET_ALL +
              ": addr=" + Fore.YELLOW + f"0x{addr:08X}" + Style.RESET_ALL +
              " length=" + Fore.YELLOW + f"{length}" + Style.RESET_ALL +
              " crc=" + Fore.YELLOW + f"0x{crc:08X}")
      else:
        print(Fore.BLUE + f" {seqnum}" + Style.RESET_ALL +
              ": cmd=" + Fore.YELLOW + f"{cmd}" + Style.RESET_ALL +
              " addr=" + Fore.YELLOW + f"0x{addr:08X}({addr})" + Style.RESET_ALL +
              " length=" + Fore.YELLOW + f"{length}" + Style.RESET_ALL +
              " crc=" + Fore.YELLOW + f"0x{crc:08X}")

      payload = file.read(length)
      if crc32(payload):
        error("Payload: invalid CRC")
        break

main()
