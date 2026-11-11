#include <stdio.h>
#include "pico/status_led.h"
#include "pico/stdlib.h"
#include "button.h"
#include "display.h"
#include "pt6315.h"

static int s_state;
static uint8_t s_segm[2];

static void on_button(void) {
  status_led_set_state(!(s_state % 2));

  printf("seg=%u\n", s_state);

  if (s_state < 8) {
    s_segm[0] = 1 << s_state;
    s_segm[1] = 0;
  } else {
    s_segm[0] = 0;
    s_segm[1] = 1 << s_state;
  }

  pt6315_write_block(6, s_segm, 2);
  s_state++;
}

int main() {
  stdio_init_all();

  // Initialize hardware modules
  status_led_init();
  button_init(PICO_DEFAULT_BUTTON_PIN);
  button_set_callback(on_button);

  while (!stdio_usb_connected())
    sleep_ms(10);
  sleep_ms(200);

  pt6315_init(16, 21, 29, 28); // STB, CLK, DIN, DOUT
  pt6315_set_mode(PT6315_GRID_9);
  pt6315_clear_block(0, VFD_GRIDS * 3);
  pt6315_display(true, PT6315_DUTY_14_16);
  pt6315_set_leds(0x02);

  while (true) {
    // Background tasks (non-blocking)
    button_task_update();

    // Small sleep or let it run free-spin with tight loop
    tight_loop_contents();
  }
}
