#include <stdio.h>
#include <string.h>
#include "pico/status_led.h"
#include "pico/stdlib.h"
#include "button.h"
#include "et16315.h"
#include "vfd.h"

static int s_state;

static void on_button(void) {
static const char all[] =
    "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZ012345"
    "6789m+*/" "\\_=<>.- ";   /* 48 znakov = 6 riadkov po 8 */

  char buf[9];
  memcpy(buf, &all[(s_state % 6) * 8], 8);
  buf[8] = '\0';

  status_led_set_state(!(s_state % 2));

  vfd_draw_text(buf, 0);
  vfd_flush();
  s_state++;
  return;

  status_led_set_state(!(s_state % 2));

//  vfd_draw_num(-1234, 2);
//  vfd_draw_text("mA", 6);
//  vfd_flush();

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
    // Background tasks (non-blocking)
    button_task_update();

    // Small sleep or let it run free-spin with tight loop
    tight_loop_contents();
  }
}
