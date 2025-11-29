import sys, time, struct
import usb.core, usb.util
import subprocess
from pathlib import Path
from colorama import init, Fore, Style

init(autoreset=True)

def info(msg: str):
  print(Fore.CYAN + " " + msg)

def warn(msg: str):
  print(Fore.YELLOW + "" + msg)

def error(msg: str):
  print(Fore.RED + "❌" + msg)

marvell_iface = 0
marvell_ep_cmd = 1

def init() -> usb.core.Device:
  dev = usb.core.find(idVendor=0x1286, idProduct=0x203c)
  if dev is None:
    error("Marvell 88W8786U device not found")
    return None

  try:
    if dev.is_kernel_driver_active(marvell_iface):
      dev.detach_kernel_driver(marvell_iface)
  except:
    error("Kernel driver detach failed")

  try:
    usb.util.claim_interface(dev, marvell_iface)
  except:
    error("Claim interface failed")
    return None

  try:
    dev.reset()
    time.sleep(0.5)
  except:
    error("Device reset failed")
    return None

  return dev

def release(dev: usb.core.Device):
  if dev is None:
    return

  try:
    usb.util.release_interface(dev, marvell_iface)
  except:
    error("Release interface failed")

  try:
    usb.util.dispose_resources(dev)
  except:
    pass

def testpacket(dev: usb.core.Device, seqnum: int):
  packet = "cefa0df00700000000000000"
  data = bytes.fromhex(packet)
  print("Sending length:", len(data))
  print("Sending hex:", packet)
  dev.write(marvell_ep_cmd, data, timeout=100)

  response = dev.read(marvell_ep_cmd | 0x80, 2048, timeout=5000)
  print("Response length:", len(response))
  print("Response hex:", bytes(response).hex())

def cmd2str(cmd: int) -> str:
  if cmd == 1:
    return "DNLD"
  if cmd == 4:
    return "LAST"
  return f"CMD{cmd}"

def sendPacket(dev: usb.core.Device, data: bytes) -> bool:
  try:
    written = dev.write(marvell_ep_cmd, data, timeout=100)
  except:
    error("Operation timed out")
    return False

  fw_hdr = data[:20]
  cmd, addr, length, crc, seqnum = struct.unpack("<5I", fw_hdr)
  if crc:
    cmdstr = cmd2str(cmd)
    print(Fore.BLUE + f"{seqnum} {cmdstr}")
    print(" Sent header:" +
          " addr=" + Fore.YELLOW + f"0x{addr:08X}({addr})" + Style.RESET_ALL +
          " length=" + Fore.YELLOW + f"{length}" + Style.RESET_ALL +
          " crc=" + Fore.YELLOW + f"0x{crc:08X}")
  else:
    info("Send pseudo data to check winner status first")
    
  written -= 20
  if written:
    print(f" Sent payload: " + Fore.YELLOW + f"{written}" + Style.RESET_ALL + " bytes")

  try:
    resp = dev.read(marvell_ep_cmd | 0x80, 2048, timeout=100)
    sync_fw = resp[:8]
    cmd, seqnum = struct.unpack("<II", sync_fw)

    if cmd:
      icon = Fore.RED + "❌"
    else:
      icon = Fore.GREEN + "✅"

    if seqnum > 2000:
      errcode = " status=" + Fore.YELLOW + f"0x{seqnum:08X}"
    else:
      errcode = " seqnum=" + Fore.YELLOW + f"{seqnum}"
    print(icon + Style.RESET_ALL + " Response: " + Fore.YELLOW + f"{resp.tobytes().hex()}" + Style.RESET_ALL + errcode)
#    if cmd & 0x80000000:
#      warn("USB is not the winner")
#    if cmd:
#      error("FW received packet with CRC")
  except:
    error("Operation timed out")
    return False

  return True

def readPacket(dev: usb.core.Device, file, seqnum: int) -> bool:
  header = file.read(16)  # 4x DWORD = 16 bytes
  if len(header) == 0:
    return False

  data = header + struct.pack("<I", seqnum)
  cmd, addr, length, crc = struct.unpack("<4I", header)
  if cmd == 1:
    data += file.read(length)

  if not sendPacket(dev, data):
    sendPacket(dev, data)
  return True

def read_blob(dev: usb.core.Device, id: int, seqnum: int) -> int:
  path = Path(f"packet/{id}")
  if not path.exists():
    error(f"File '{path}' not found")
    return -1

  # pseudo data
  if id == 1000:
    info("Data:")
  elif id == 1001:
    info("Small:")
  elif id == 1002:
    info("Firmware:")

  with open(path, "rb") as file:
    while True:
      if not readPacket(dev, file, seqnum):
        break
      seqnum += 1;

  return seqnum

def print_id():
  result = subprocess.run("lsusb | grep 88W8786U | awk '{print $6}'", shell=True, capture_output=True, text=True)
  info("Marvell VID:PID -> " + result.stdout.strip())

def main():
  dev = init()
  if dev == None:
    return

  print_id()
#  blobs = [0, 1, 1, 5, 6, 7, 10, 21, 4]
  blobs = [1000, 1001, 1002, 4]
  seqnum = 0

  for id in blobs:
    seqnum = read_blob(dev, id, seqnum)
    if seqnum == -1:
      return

  testpacket(dev, seqnum)
  release(dev)
  print_id()

main()
