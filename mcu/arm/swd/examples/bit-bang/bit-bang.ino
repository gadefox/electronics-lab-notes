// swd_poc.ino - minimalny SWD master pre LGT8F328P / ATmega328 (5 V logika)
// Proof of concept: zistit, ci MCU (Cortex-M0) odpoveda, ci drzi reset a ci ide halt.
//
// Zapojenie:  D2 -> SWCLK,  D3 -> SWDIO,  D4 -> nRESET (volitelne),  GND -> GND
// Odporucam 100-220 ohm do serie na SWCLK a SWDIO.
// MCU napajaj z 5 V (z LGT alebo z vlastneho zdroja, nie z oboch naraz).
//
// Seriova konzola 115200, koniec riadku \n. Prikazy (hex bez 0x):
//   i            init: line reset, JTAG->SWD, DPIDR, power-up debug
//   d            precitaj DHCSR
//   h            halt (DHCSR = A05F0003) a vypis DHCSR
//   r <addr>     citaj 32-bit slovo
//   w <addr> <v> zapis 32-bit slovo
//   t            test: DHCSR 3x, SRAM zapis/citanie, PDID (0x50000000)
//   u            connect under reset: nRESET dole, init, VC_CORERESET + halt, pust reset
//   g            dump: init, halt, R0-R3, SP, LR, PC, xPSR, flash 0/4/8, CONFIG0/1, SRAM test
//   c            precitaj CONFIG0/1 cez FMC ISP (+ test zapisu do PPB a SRAM 0x20001000)
//   e yes        MASS ERASE (APROM, LDROM, CONFIG, zrusi LOCK a BOD v CONFIG0!) cez ISP 0x26
//   p <addr>     page erase 512 B (ISP 0x22), addr zarovnana na 0x200
//   f <addr> <h> programuj slova (ISP 0x21); h = hex bajty (4*n, max 32 B), addr zarovnana na 4
//   v <addr> <h> over obsah pamate proti hex bajtom
//   x            pust halt a urob SYSRESETREQ (spusti FW)
//   a / n        nRESET dole / uvolnit (D4)

#include <Arduino.h>

#define SWCLK_PIN 2
#define SWDIO_PIN 3
#define NRST_PIN  4

#define DHCSR_ADDR 0xE000EDF0UL
#define DEMCR_ADDR 0xE000EDFCUL
#define DCRSR_ADDR 0xE000EDF4UL
#define DCRDR_ADDR 0xE000EDF8UL
#define REGWRPROT_ADDR 0x50000100UL
#define AHBCLK_ADDR    0x50000204UL
#define ISPCON_ADDR    0x5000C000UL
#define ISPADR_ADDR    0x5000C004UL
#define ISPDAT_ADDR    0x5000C008UL
#define ISPCMD_ADDR    0x5000C00CUL
#define ISPTRG_ADDR    0x5000C010UL
#define ISPCHEAT_ADDR  0x5000C01CUL   // nedokumentovany ISP register (OpenOCD ho nastavuje na 1)

#define ACK_OK    1
#define ACK_WAIT  2
#define ACK_FAULT 4
#define ACK_PARITY 8   // vlastny kod: chyba parity pri citani

static inline void clkLow()  { digitalWrite(SWCLK_PIN, LOW);  }
static inline void clkHigh() { digitalWrite(SWCLK_PIN, HIGH); }
static inline void dioOut()  { pinMode(SWDIO_PIN, OUTPUT); }
static inline void dioIn()   { pinMode(SWDIO_PIN, INPUT_PULLUP); }

// host zapisuje: data nastav, clk dole, clk hore (target vzorkuje na nabeznej hrane)
static void writeBit(uint8_t b) {
  digitalWrite(SWDIO_PIN, b);
  clkLow();
  clkHigh();
}

// host cita: clk dole, vzorkuj, clk hore
static uint8_t readBit() {
  clkLow();
  uint8_t b = digitalRead(SWDIO_PIN) ? 1 : 0;
  clkHigh();
  return b;
}

static void cycle() {
  clkLow();
  clkHigh();
}

static void writeBits(uint32_t v, uint8_t n) {
  for (uint8_t i = 0; i < n; i++) writeBit((v >> i) & 1);
}

static void lineReset() {
  dioOut();
  for (uint8_t i = 0; i < 60; i++) writeBit(1);
}

// jeden SWD prenos; vrati ACK (1 = OK, 2 = WAIT, 4 = FAULT, 7/0 = ziadna odpoved)
static uint8_t swdXfer(bool ap, bool rd, uint8_t addr, uint32_t &data) {
  uint8_t a2 = (addr >> 2) & 1;
  uint8_t a3 = (addr >> 3) & 1;
  uint8_t par = (uint8_t)(ap ^ rd ^ a2 ^ a3) & 1;
  uint8_t req = 1 | ((uint8_t)ap << 1) | ((uint8_t)rd << 2) | (a2 << 3) | (a3 << 4) | (par << 5) | (1 << 7);

  dioOut();
  writeBits(req, 8);
  dioIn();
  cycle();                         // turnaround
  uint8_t ack = 0;
  for (uint8_t i = 0; i < 3; i++) ack |= readBit() << i;

  if (ack == ACK_OK) {
    if (rd) {
      uint32_t v = 0;
      uint8_t p = 0;
      for (uint8_t i = 0; i < 32; i++) {
        uint8_t b = readBit();
        v |= (uint32_t)b << i;
        p ^= b;
      }
      uint8_t pb = readBit();
      cycle();                     // turnaround
      dioOut();
      data = v;
      if ((p & 1) != pb) ack = ACK_PARITY;
    } else {
      cycle();                     // turnaround
      dioOut();
      uint8_t p = 0;
      for (uint8_t i = 0; i < 32; i++) {
        uint8_t b = (data >> i) & 1;
        writeBit(b);
        p ^= b;
      }
      writeBit(p & 1);
    }
  } else {
    cycle();                       // turnaround
    dioOut();
  }
  for (uint8_t i = 0; i < 8; i++) writeBit(0);   // idle
  return ack;
}

static uint8_t xfer(bool ap, bool rd, uint8_t addr, uint32_t &data) {
  uint8_t ack = 0;
  for (uint8_t t = 0; t < 20; t++) {
    ack = swdXfer(ap, rd, addr, data);
    if (ack != ACK_WAIT) break;
  }
  if (ack == ACK_FAULT) {          // zmaz sticky chyby (ABORT)
    uint32_t a = 0x1E;
    swdXfer(false, false, 0x00, a);
  }
  return ack;
}

static bool memRead(uint32_t addr, uint32_t &val) {
  uint32_t t = addr, d = 0;
  if (xfer(true, false, 0x04, t) != ACK_OK) return false;   // TAR
  if (xfer(true, true, 0x0C, d) != ACK_OK) return false;    // DRW (posted)
  if (xfer(false, true, 0x0C, val) != ACK_OK) return false; // RDBUFF
  return true;
}

static bool memWrite(uint32_t addr, uint32_t val) {
  uint32_t t = addr, v = val, d = 0;
  if (xfer(true, false, 0x04, t) != ACK_OK) return false;  // TAR
  if (xfer(true, false, 0x0C, v) != ACK_OK) return false;  // DRW
  if (xfer(false, true, 0x0C, d) != ACK_OK) return false;  // RDBUFF, dokonci zapis
  return true;
}

static void hex32(uint32_t v) {
  Serial.print(F("0x"));
  for (int8_t i = 28; i >= 0; i -= 4) Serial.print((v >> i) & 0xF, HEX);
}

static void printHex(const __FlashStringHelper *name, uint32_t v) {
  Serial.print(name);
  hex32(v);
  Serial.println();
}

// citanie registra jadra (jadro musi byt zastavene)
static bool readCoreReg(uint8_t n, uint32_t &v) {
  if (!memWrite(DCRSR_ADDR, n)) return false;
  for (uint8_t i = 0; i < 20; i++) {
    uint32_t d;
    if (memRead(DHCSR_ADDR, d) && (d & 0x10000UL)) break;   // S_REGRDY
  }
  return memRead(DCRDR_ADDR, v);
}

static bool doInit() {
  digitalWrite(SWCLK_PIN, HIGH);
  pinMode(SWCLK_PIN, OUTPUT);
  lineReset();
  writeBits(0xE79E, 16);           // JTAG -> SWD
  lineReset();
  writeBits(0, 4);                 // idle

  uint32_t id = 0;
  uint8_t ack = xfer(false, true, 0x00, id);
  Serial.print(F("DPIDR ack=")); Serial.println(ack);
  if (ack != ACK_OK) {
    Serial.println(F("Ziadna odpoved (ack 7/0 = kabel, napajanie, zle piny)"));
    return false;
  }
  printHex(F("DPIDR = "), id);

  uint32_t v = 0x1E;               // ABORT
  xfer(false, false, 0x00, v);
  v = 0x50000000;                  // CSYSPWRUPREQ | CDBGPWRUPREQ
  xfer(false, false, 0x04, v);
  uint32_t st = 0;
  for (uint8_t i = 0; i < 50; i++) {
    xfer(false, true, 0x04, st);
    if ((st & 0xA0000000UL) == 0xA0000000UL) break;
    delay(2);
  }
  printHex(F("CTRL/STAT = "), st);
  v = 0;                           // SELECT: AP 0, bank 0
  xfer(false, false, 0x08, v);
  v = 0x23000002;                  // CSW: 32-bit, bez auto-increment
  xfer(true, false, 0x00, v);
  return true;
}

static void printDhcsr() {
  uint32_t v;
  if (memRead(DHCSR_ADDR, v)) {
    printHex(F("DHCSR = "), v);
    Serial.print(F("  S_HALT=")); Serial.print((int)((v >> 17) & 1));
    Serial.print(F(" S_RESET_ST=")); Serial.println((int)((v >> 25) & 1));
  } else {
    Serial.println(F("DHCSR: citanie zlyhalo"));
  }
}

static void dumpAll() {
  if (!doInit()) return;
  if (!memWrite(DHCSR_ADDR, 0xA05F0003UL)) { Serial.println(F("halt zlyhal")); return; }
  printDhcsr();
  uint32_t v;
  Serial.println(F("registre (13=SP 14=LR 15=PC 16=xPSR):"));
  const uint8_t regs[] = {0, 1, 2, 3, 13, 14, 15, 16};
  for (uint8_t i = 0; i < sizeof(regs); i++) {
    Serial.print(F("  R")); Serial.print(regs[i]);
    if (readCoreReg(regs[i], v)) printHex(F(" = "), v);
    else Serial.println(F(" zlyhalo"));
  }
  Serial.println(F("pamat:"));
  const uint32_t addrs[] = {0x00000000UL, 0x00000004UL, 0x00000008UL, 0x00300000UL, 0x00300004UL};
  for (uint8_t i = 0; i < 5; i++) {
    Serial.print(F("  ")); hex32(addrs[i]);
    if (memRead(addrs[i], v)) printHex(F(" = "), v);
    else Serial.println(F(" zlyhalo"));
  }
  if (memWrite(0x20000000UL, 0x12345678UL) && memRead(0x20000000UL, v))
    printHex(F("  SRAM 0x20000000 po zapise 12345678 = "), v);
  else
    Serial.println(F("  SRAM test zlyhal"));
}



// priprava ISP presne ako v OpenOCD numicro drivere (vratane nedokumentovaneho registra)
// jadro musi byt zastavene
static bool ispInit() {
  uint32_t v;
  if (!memRead(REGWRPROT_ADDR, v)) return false;
  if (v == 0) {                                   // 0 = zamknute -> odomkni
    memWrite(REGWRPROT_ADDR, 0x59);
    memWrite(REGWRPROT_ADDR, 0x16);
    memWrite(REGWRPROT_ADDR, 0x88);
    if (!memRead(REGWRPROT_ADDR, v) || v != 1) {
      Serial.println(F("  chranene registre sa nepodarilo odomknut"));
      return false;
    }
  }
  if (!memRead(AHBCLK_ADDR, v)) return false;
  memWrite(AHBCLK_ADDR, v | 0x34);                // ISP_EN | SRAM_EN | TICK_EN
  if (!memRead(ISPCON_ADDR, v)) return false;
  memWrite(ISPCON_ADDR, v | 0x7D);                // ISPFF (zmaze flag) | LDUEN | CFGUEN | APUEN | SPUEN | ISPEN
  memWrite(ISPCHEAT_ADDR, 1);                     // nedokumentovany register
  return true;
}

// ISP prikaz; vrati false pri chybe (ISPFF) alebo timeoute; vysledok v rdata
static bool ispCmd(uint8_t cmd, uint32_t addr, uint32_t wdata, uint32_t &rdata, uint16_t timeoutMs) {
  uint32_t v;
  memWrite(ISPCMD_ADDR, cmd);
  memWrite(ISPDAT_ADDR, wdata);
  memWrite(ISPADR_ADDR, addr);
  memWrite(ISPTRG_ADDR, 1);                       // ISPGO
  bool done = false;
  for (uint16_t i = 0; i < timeoutMs; i++) {
    if (!memRead(ISPTRG_ADDR, v)) return false;
    if (!(v & 1)) { done = true; break; }
    delay(1);
  }
  if (!done) { Serial.println(F("  ISP timeout")); return false; }
  uint32_t con;
  if (memRead(ISPCON_ADDR, con) && (con & 0x40)) {
    printHex(F("  ISPFF, ISPCON = "), con);
    memWrite(ISPCON_ADDR, con | 0x40);            // zapis 1 zmaze flag
    return false;
  }
  return memRead(ISPDAT_ADDR, rdata);
}

static bool ispRead(uint32_t addr, uint32_t &val) {
  if (!ispInit()) return false;
  return ispCmd(0x00, addr, 0, val, 300);
}

static void configDump() {
  if (!doInit()) return;
  if (!memWrite(DHCSR_ADDR, 0xA05F0003UL)) { Serial.println(F("halt zlyhal")); return; }
  uint32_t v;
  if (ispRead(0x00300000UL, v)) {
    printHex(F("CONFIG0 = "), v);
    Serial.print(F("  LOCK (bit1) = ")); Serial.println((int)((v >> 1) & 1));
  } else Serial.println(F("CONFIG0 zlyhalo"));
  if (ispRead(0x00300004UL, v)) printHex(F("CONFIG1 = "), v);
  else Serial.println(F("CONFIG1 zlyhalo"));
  // test zapisu: PPB (DCRDR) a SRAM
  if (memWrite(DCRDR_ADDR, 0xA5A5A5A5UL) && memRead(DCRDR_ADDR, v)) printHex(F("DCRDR po zapise A5A5A5A5 = "), v);
  if (memWrite(0x20001000UL, 0x5A5A5A5AUL) && memRead(0x20001000UL, v)) printHex(F("SRAM 0x20001000 po zapise 5A5A5A5A = "), v);
  else Serial.println(F("SRAM zapis/citanie zlyhalo"));
}


// mass erase cez nedokumentovany ISP prikaz 0x26 (OpenOCD: "numicro chip_erase")
// zmaze APROM, LDROM aj CONFIG -> LOCK sa zrusi; CONFIG0 sa vrati na 0xFFFFFFFF
static void eraseAll() {
  if (!doInit()) return;
  if (!memWrite(DHCSR_ADDR, 0xA05F0003UL)) { Serial.println(F("halt zlyhal")); return; }
  uint32_t v;
  if (!ispInit()) { Serial.println(F("ISP init zlyhal")); return; }
  if (ispCmd(0x26, 0, 0, v, 3000)) {
    Serial.println(F("mass erase (0x26) dobehol"));
  } else {
    Serial.println(F("mass erase (0x26) zlyhal, skusam page erase CONFIG (0x22 @ 0x00300000)"));
    if (ispInit() && ispCmd(0x22, 0x00300000UL, 0, v, 500)) Serial.println(F("page erase CONFIG dobehol"));
    else Serial.println(F("page erase CONFIG zlyhal"));
  }
  if (ispRead(0x00300000UL, v)) {
    printHex(F("CONFIG0 po erase = "), v);
    Serial.print(F("  LOCK (bit1) = ")); Serial.println((int)((v >> 1) & 1));
  } else Serial.println(F("CONFIG0 zlyhalo"));
  Serial.println(F("Teraz vypni a zapni napajanie MCU a spusti c a g."));
}


static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

// "aabbcc" -> bajty; vrati pocet bajtov alebo -1 pri chybe
static int parseHex(const char *s, uint8_t *out, uint8_t maxn) {
  uint8_t n = 0;
  while (*s == ' ') s++;
  while (s[0] && s[1]) {
    int a = hexVal(s[0]), b = hexVal(s[1]);
    if (a < 0 || b < 0) break;
    if (n >= maxn) return -1;
    out[n++] = (uint8_t)((a << 4) | b);
    s += 2;
  }
  return n;
}

static uint32_t le32(const uint8_t *b) {
  return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static void cmdPageErase(const char *s) {
  uint32_t a = strtoul(s, NULL, 16), v;
  if (a & 0x1FF) { Serial.println(F("err zle zarovnanie (512 B)")); return; }
  if (!ispInit()) { Serial.println(F("err isp init")); return; }
  Serial.println(ispCmd(0x22, a, 0, v, 200) ? F("ok") : F("err page erase"));
}

// f <addr> <hex>
static void cmdProgram(const char *s) {
  char *p;
  uint32_t a = strtoul(s, &p, 16), v;
  uint8_t buf[32];
  int n = parseHex(p, buf, sizeof(buf));
  if ((a & 3) || n <= 0 || (n & 3)) { Serial.println(F("err argumenty")); return; }
  if (!ispInit()) { Serial.println(F("err isp init")); return; }
  for (int i = 0; i < n; i += 4) {
    if (!ispCmd(0x21, a + i, le32(buf + i), v, 50)) {
      Serial.print(F("err program @")); hex32(a + i); Serial.println();
      return;
    }
  }
  Serial.println(F("ok"));
}

// v <addr> <hex>
static void cmdVerify(const char *s) {
  char *p;
  uint32_t a = strtoul(s, &p, 16), v;
  uint8_t buf[32];
  int n = parseHex(p, buf, sizeof(buf));
  if ((a & 3) || n <= 0 || (n & 3)) { Serial.println(F("err argumenty")); return; }
  for (int i = 0; i < n; i += 4) {
    if (!memRead(a + i, v)) { Serial.println(F("err citanie")); return; }
    uint32_t e = le32(buf + i);
    if (v != e) {
      Serial.print(F("diff @")); hex32(a + i);
      Serial.print(F(" got ")); hex32(v);
      Serial.print(F(" exp ")); hex32(e); Serial.println();
      return;
    }
  }
  Serial.println(F("ok"));
}

static void cmdRun() {
  memWrite(DHCSR_ADDR, 0xA05F0000UL);            // zrus halt aj debug enable
  memWrite(0xE000ED0CUL, 0x05FA0004UL);          // AIRCR: SYSRESETREQ
  Serial.println(F("reset odoslany, MCU by mal bezat (ak nie, vypni a zapni napajanie)"));
}

static void nrstLow()  { digitalWrite(NRST_PIN, LOW); pinMode(NRST_PIN, OUTPUT); }
static void nrstFree() { pinMode(NRST_PIN, INPUT); }

void setup() {
  Serial.begin(115200);
  digitalWrite(SWCLK_PIN, HIGH);
  pinMode(SWCLK_PIN, OUTPUT);
  dioIn();
  nrstFree();
  Serial.println(F("SWD PoC pripraveny: i, d, h, r, w, t, u, g, c, e, p, f, v, x, a, n"));
}

static void handleLine(String line) {
  line.trim();
  if (!line.length()) return;
  const char *s = line.c_str() + 1;
  char *p;
  uint32_t v;

  switch (line[0]) {
    case 'i':
      doInit();
      break;
    case 'd':
      printDhcsr();
      break;
    case 'h':
      if (memWrite(DHCSR_ADDR, 0xA05F0003UL)) printDhcsr();
      else Serial.println(F("zapis DHCSR zlyhal"));
      break;
    case 'r': {
      uint32_t a = strtoul(s, NULL, 16);
      if (memRead(a, v)) printHex(F("= "), v);
      else Serial.println(F("citanie zlyhalo"));
      break;
    }
    case 'w': {
      uint32_t a = strtoul(s, &p, 16);
      uint32_t val = strtoul(p, NULL, 16);
      Serial.println(memWrite(a, val) ? F("ok") : F("zapis zlyhal"));
      break;
    }
    case 't':
      if (!doInit()) break;
      for (uint8_t i = 0; i < 3; i++) printDhcsr();
      if (memWrite(0x20000000UL, 0x12345678UL) && memRead(0x20000000UL, v))
        printHex(F("SRAM 0x20000000 = "), v);
      else
        Serial.println(F("SRAM test zlyhal"));
      if (memRead(0x50000000UL, v)) printHex(F("PDID 0x50000000 = "), v);
      else Serial.println(F("PDID zlyhalo"));
      break;
    case 'u':
      nrstLow();
      delay(10);
      if (doInit()) {
        memWrite(DEMCR_ADDR, 1);                 // VC_CORERESET
        memWrite(DHCSR_ADDR, 0xA05F0003UL);      // halt + debug enable
      }
      nrstFree();
      delay(50);
      printDhcsr();
      break;
    case 'g':
      dumpAll();
      break;
    case 'c':
      configDump();
      break;
    case 'e':
      if (line == "e yes") eraseAll();
      else Serial.println(F("MASS ERASE zmaze cely flash vratane CONFIG. Potvrd prikazom: e yes"));
      break;
    case 'p':
      cmdPageErase(s);
      break;
    case 'f':
      cmdProgram(s);
      break;
    case 'v':
      cmdVerify(s);
      break;
    case 'x':
      cmdRun();
      break;
    case 'a':
      nrstLow();
      Serial.println(F("nRESET dole"));
      break;
    case 'n':
      nrstFree();
      Serial.println(F("nRESET uvolneny"));
      break;
    default:
      Serial.println(F("? prikazy: i d h r w t u g c e p f v x a n"));
  }
}

// riadok sa sklada po znakoch bez timeoutu: funguje aj pri rucnom pisani v tio ("r 8000" + Enter)
static String lineBuf;

void loop() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (lineBuf.length()) {
        String l = lineBuf;
        lineBuf = "";
        handleLine(l);
      }
    } else if (lineBuf.length() < 80) {
      lineBuf += ch;
    }
  }
}
