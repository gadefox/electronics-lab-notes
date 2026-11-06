#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/status_led.h"
#include "button.h"
#include "display.h"

bool s_state;

static void render(void) {
  lcd_fb_draw_corners();
  lcd_fb_draw_diamond(s_state);
  lcd_flush();

  status_led_set_state(s_state);
}

static void on_button(void) {
  s_state ^= 1;
  render();
}

int main() {
  stdio_init_all();
  status_led_init();

  button_init(PICO_DEFAULT_BUTTON_PIN);
  button_set_callback(on_button);

  lcd_init(4, 5, 10); // SDA, SCL, RST
  lcd_reset();
  lcd_init_seq();
  render();

  while (true) {
    button_task_update();
    tight_loop_contents();
  }
}
