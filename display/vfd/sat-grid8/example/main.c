#include <stdio.h>
#include "pico/status_led.h"
#include "pico/stdlib.h"
#include "button.h"
#include "et16315.h"
#include "vfd.h"

static int s_state;
static uint8_t s_segm[3];

static void on_button(void) {
  status_led_set_state(!(s_state % 2));

  printf("seg=%u\n", s_state);

  if (s_state < 8) {
    s_segm[0] = 1 << s_state;
    s_segm[1] = 0;
    s_segm[2] = 0;
  } else if (s_state < 16) {
    s_segm[0] = 0;
    s_segm[1] = 1 << (s_state - 8);
    s_segm[2] = 0;
  } else if (s_state < 24) {
    s_segm[0] = 0;
    s_segm[1] = 0;
    s_segm[2] = 1 << (s_state - 16);
  }

  et16315_write_block(8*3, s_segm, 3);

/*
  s_segm[0] = 1;
  s_segm[1] = 0;
  s_segm[2] = 0;

  et16315_write_block(s_state * 3, s_segm, 3);
*/
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

  vfd_init(16, 17, 18, 19); // STB, CLK, DOUT, DIN
  et16315_set_leds(0x02);

  while (true) {
    // Background tasks (non-blocking)
    button_task_update();

    // Small sleep or let it run free-spin with tight loop
    tight_loop_contents();
  }
}
