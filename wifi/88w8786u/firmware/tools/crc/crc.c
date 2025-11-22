#include "crc.h"

#ifndef COUNTOF
#define COUNTOF(x)  (sizeof(x) / sizeof((x)[0]))
#endif

static uint32_t crc_polynomials[UINT8_MAX + 1];

static uint32_t crc_polynomial(uint32_t remainder) {
  uint32_t bit = 8;
 
  do {
    if (remainder & 0x80000000) {
      remainder <<= 1;
      remainder ^= 0x04C11DB7;
    } else
      remainder <<= 1;
  } while (--bit != 0);

  return remainder;
}

void crc_init(void) {
  for (uint32_t dividend = 0; dividend < COUNTOF(crc_polynomials); dividend++)
    crc_polynomials[dividend] = crc_polynomial(dividend << 24);
}

uint32_t crc_calc(uint32_t remainder, const uint8_t *data, uint32_t len) {
  while (len != 0) {
    uint8_t index = *data++ ^ (remainder >> 24);
    remainder = crc_polynomials[index] ^ (remainder << 8);
    len--;
  }
  return remainder;
}
