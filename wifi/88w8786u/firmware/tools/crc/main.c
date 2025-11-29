#include <stdio.h>
#include "crc.h"

static void crc_test(uint32_t cmd, uint32_t addr, uint32_t len, uint32_t crc)
{
  uint32_t data[4] = { cmd, addr, len, crc };
  uint32_t calc;

  printf("Target: 0x%08X (cmd=%d)\n", crc, cmd);

  calc = crc_calc(0, (uint8_t*)data, sizeof(data) - sizeof(uint32_t));
  printf("CRC: 0x%08X\n", __builtin_bswap32(calc));

  calc = crc_calc(0, (uint8_t*)data, sizeof(data));
  printf("Test: %d\n\n", calc);
}

static uint32_t packet_read(uint8_t *buf, size_t len) {
  FILE *file = fopen("1", "rb");
  if (file == NULL) {
    perror("ERROR: file does not exist");
    return 0;
  }

  size_t n = fread(buf, 1, len, file);
  if (n != len)
    fprintf(stderr, "read %zu bytes (expected %zu)\n", n, len);

  fclose(file);
  return 1;
}

int main() {
  uint32_t calc;
  uint8_t buf[528];
  uint32_t* data = (uint32_t*)buf;

  crc_init();
  printf("Test: 0=OK\n\n");

  /* TEST: no payload */
  crc_test(0, 0, 0, 0);
  crc_test(4, 0, 0, 0x188CDB1F);
  crc_test(5, 0, 12, 0x11BB329D);
  crc_test(6, 0, 0, 0x144A3610);
  crc_test(6, 0x00080A4D, 0, 0x39416A7F);
  crc_test(6, 0x00080CF9, 0, 0xAEE2B639);
  crc_test(6, 0x00080D2D, 0, 0x9AB29F76);
  crc_test(6, 0x00080ECD, 0, 0xF9A9F63F);
  crc_test(6, 0x00080F2D, 0, 0xB86231E8);
  crc_test(7, 3, 3, 0x227E559B);
  crc_test(10, 0, 0, 0x3CDE5A30);
  crc_test(21, 0, 0, 0x7E5F4367);
  crc_test(99, 0, 0, 0xFD99BE0F);

  /* read test packet file */
  if (packet_read(buf, sizeof(buf))) {
    /* TEST: with payload */
    printf("Target: 0x%08X\n", data[3]);
    calc = crc_calc(0, buf, 12);
    printf("CRC: 0x%08X\n", __builtin_bswap32(calc));
    calc = crc_calc(0, buf, 16);
    printf("Test header: %d\n", calc);
    calc = crc_calc(0, buf + 16, 512);
    printf("Test payload: %d\n", calc);
  }

  return 0;
}
