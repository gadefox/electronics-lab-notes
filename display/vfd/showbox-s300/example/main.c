#include <stdio.h>
#include <string.h>
#include "pico/status_led.h"
#include "pico/stdlib.h"
#include "button.h"
#include "et16315.h"
#include "vfd.h"

static int s_state;

static void on_button(void) {
  status_led_set_state(!(s_state % 2));

  if (s_state % 2) {
    vfd_set_icons(VFD_ICON_ONE);
    vfd_draw_num(-2345, 3);
    vfd_draw_text(" V", 6);
  } else {
    vfd_set_icons(VFD_ICON_TWO);
    vfd_draw_num(6789, 2);
    vfd_draw_text("mA", 6);
  }

  vfd_flush();
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
  vfd_draw_text("HELLO W.", 0);
  vfd_flush();

  while (true) {
    uint8_t keys = 0;
    et16315_read_keys(&keys, 1);
    if (keys)
      printf("KS: %x\n", keys);

    // Background tasks (non-blocking)
    button_task_update();

    // Small sleep or let it run free-spin with tight loop
    tight_loop_contents();
  }
}
