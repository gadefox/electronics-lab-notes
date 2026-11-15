#include <stdio.h>
#include "pico/stdlib.h"

#define IR_PIN  23

static uint32_t measure(bool level, uint32_t timeout_us) {
  uint64_t t0 = time_us_64();
  while (gpio_get(IR_PIN) == level) {
    if (time_us_64() - t0 > timeout_us)
      return 0;
  }
  return (uint32_t)(time_us_64() - t0);
}

int main() {
  stdio_init_all();

  gpio_init(IR_PIN);
  gpio_set_dir(IR_PIN, GPIO_IN);

  while (!stdio_usb_connected())
    sleep_ms(10);
  sleep_ms(200);

  while (true) {
    while (gpio_get(IR_PIN))
      tight_loop_contents();

    uint32_t m = measure(0, 12000);
    if (m < 8000 || m > 10000)
      continue;
    uint32_t s = measure(1, 6000);
    if (s < 3800 || s > 5200)
      continue;

    uint32_t data = 0;
    bool ok = true;
    for (int i = 0; i < 32; i++) {
      uint32_t mark  = measure(0, 1000);  // ~560 us
      uint32_t space = measure(1, 2500);  // 560 us = 0, 1690 us = 1
      if (!mark || !space) {
        ok = false;
        break;
      }
      if (space > 1125)
        data |= (1u << i);          // LSB first
    }

    if (!ok)
      continue;

    uint8_t addr  =  data        & 0xFF;
    uint8_t naddr = (data >> 8)  & 0xFF;
    uint8_t cmd   = (data >> 16) & 0xFF;
    uint8_t ncmd  = (data >> 24) & 0xFF;
    printf("addr=0x%02X cmd=0x%02X %s\n", addr, cmd,
           (cmd == (uint8_t)~ncmd && addr == (uint8_t)~naddr) ? "OK" : "error");
  }
}
