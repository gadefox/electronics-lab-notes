#include "pico/stdlib.h"
#include "ws2812.pio.h"

#define WS2812_PIN 23
#define COUNTOF(x)  (sizeof(x) / sizeof(x[0]))

uint32_t colors[32];

inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (g << 24) | (r << 16) | (b << 8);
}

uint32_t hsv_to_rgb(float h, float s, float v) {
  float r = 0, g = 0, b = 0;
  int i = (int)(h * 6);
  float f = h * 6 - i;
  float p = v * (1 - s);
  float q = v * (1 - f * s);
  float t = v * (1 - (1 - f) * s);

  switch (i % 6) {
    case 0:
      r = v; g = t; b = p;
      break;
    case 1:
      r = q; g = v; b = p;
      break;
    case 2:
      r = p; g = v; b = t;
      break;
    case 3:
      r = p; g = q; b = v;
      break;
    case 4:
      r = t; g = p; b = v;
      break;
    case 5:
      r = v; g = p; b = q;
      break;
  }
  return rgb( r * 10, g * 10, b * 10);
}

void init_colors() {
  float saturation = 0.7f;
  float value = 0.8f;

  for (int i = 0; i < COUNTOF(colors); i++) {
    float hue = (float)i / COUNTOF(colors);
    colors[i] = hsv_to_rgb(hue, saturation, value);
  }
}

int main() {
  PIO pio;
  uint sm, offset, idx = 0;

  stdio_init_all();
  init_colors();

  bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
                 &ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
  hard_assert(success);

  ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000);

  while (1) {
    uint32_t color = colors[idx++ % COUNTOF(colors)];
    pio_sm_put_blocking(pio, sm, color);
    sleep_ms(5000 / COUNTOF(colors));
  }

  pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
