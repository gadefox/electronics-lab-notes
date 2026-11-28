#!/usr/bin/env python3
"""Nahra .bin do APROM Nuvoton MCU cez swd_poc.ino na LGT328.

Pouzitie:
    python3 flash_swd.py firmware.bin                  # /dev/ttyACM0, adresa 0
    python3 flash_swd.py firmware.bin -p /dev/ttyUSB0 -a 0 --run

Postup: init SWD, halt, page erase (512 B) pre stranky obrazu, programovanie
16 B blokov (bloky samych 0xFF sa preskocia), verifikacia, volitelne spustenie.
Vyzaduje: pip install pyserial
"""
import argparse
import sys
import time

import serial

PAGE = 512
CHUNK = 16


def read_until(ser, ok_prefixes, timeout=3.0):
    """Cita riadky, kym neprijde riadok zacinajuci niektorym z ok_prefixes
    (alebo 'err'/'diff'/'Ziadna'). Vrati posledny riadok."""
    end = time.time() + timeout
    last = ""
    while time.time() < end:
        raw = ser.readline()
        if not raw:
            continue
        line = raw.decode("ascii", errors="replace").strip()
        if not line:
            continue
        last = line
        if line.startswith(("err", "diff", "Ziadna")):
            return line
        if any(line.startswith(p) or p in line for p in ok_prefixes):
            return line
    raise TimeoutError("timeout, posledny riadok: %r" % last)


def wait_banner(ser, timeout=6.0):
    """Doska sa pri otvoreni portu resetuje (DTR) a bootloader chvilu trva.
    Pocka na uvodny riadok sketchu, inak by sa prve prikazy stratili."""
    end = time.time() + timeout
    while time.time() < end:
        if b"SWD PoC" in ser.readline():
            return True
    return False


def cmd(ser, text, ok_prefixes, timeout=3.0):
    ser.reset_input_buffer()
    ser.write((text + "\n").encode("ascii"))
    return read_until(ser, ok_prefixes, timeout)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bin")
    ap.add_argument("-p", "--port", default="/dev/ttyACM0")
    ap.add_argument("-a", "--addr", type=lambda x: int(x, 0), default=0)
    ap.add_argument("--run", action="store_true", help="po nahrati spusti FW")
    args = ap.parse_args()

    data = bytearray(open(args.bin, "rb").read())
    if args.addr % PAGE:
        sys.exit("adresa musi byt zarovnana na 512 B")
    while len(data) % CHUNK:
        data.append(0xFF)
    print("obraz: %d B na 0x%08X" % (len(data), args.addr))

    ser = serial.Serial(args.port, 115200, timeout=0.3)
    if wait_banner(ser):
        time.sleep(0.2)
    else:
        print("banner neprisiel (doska sa asi nereset), pokracujem")

    r = ""
    for _ in range(3):
        try:
            r = cmd(ser, "i", ["CTRL/STAT"], 4.0)
            break
        except TimeoutError as e:
            print("init bez odpovede, skusam znova:", e)
    else:
        sys.exit("init zlyhal (skontroluj, ci ma LGT nahraty aktualny swd_poc.ino)")
    if r.startswith("Ziadna"):
        sys.exit("SWD neodpoveda: " + r)
    r = cmd(ser, "h", ["S_HALT="], 4.0)
    print(r)
    if "S_HALT=1" not in r:
        sys.exit("halt nepresiel")

    first_page = args.addr // PAGE
    last_page = (args.addr + len(data) - 1) // PAGE
    for pg in range(first_page, last_page + 1):
        r = cmd(ser, "p %08X" % (pg * PAGE), ["ok"], 3.0)
        if r != "ok":
            sys.exit("page erase 0x%08X: %s" % (pg * PAGE, r))
    print("zmazanych stranok: %d" % (last_page - first_page + 1))

    chunks = [(args.addr + o, data[o:o + CHUNK]) for o in range(0, len(data), CHUNK)]
    todo = [(a, c) for a, c in chunks if any(b != 0xFF for b in c)]
    for n, (a, c) in enumerate(todo, 1):
        r = cmd(ser, "f %08X %s" % (a, c.hex()), ["ok"], 3.0)
        if r != "ok":
            sys.exit("\nprogramovanie 0x%08X: %s" % (a, r))
        if n % 8 == 0 or n == len(todo):
            print("\rprogramovanie %d/%d" % (n, len(todo)), end="", flush=True)
    print()

    for n, (a, c) in enumerate(chunks, 1):
        r = cmd(ser, "v %08X %s" % (a, c.hex()), ["ok"], 3.0)
        if r != "ok":
            sys.exit("\nverifikacia 0x%08X: %s" % (a, r))
        if n % 8 == 0 or n == len(chunks):
            print("\rverifikacia %d/%d" % (n, len(chunks)), end="", flush=True)
    print("\nverifikacia OK")

    if args.run:
        print(cmd(ser, "x", ["reset"], 3.0))


if __name__ == "__main__":
    main()
