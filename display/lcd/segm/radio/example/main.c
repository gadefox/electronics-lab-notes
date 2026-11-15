#include "pico/status_led.h"
#include "pico/stdlib.h"
#include "button.h"
#include "lcd.h"
#include "vk1s56e.h"

static uint16_t s_num = 320;

static void render(void) {
  lcd_draw_num(s_num);
  lcd_flush();
}

static void on_button(void) {
  s_num++;
  status_led_set_state(s_num % 2);
  render();
}

int main() {
  stdio_init_all();

  // Initialize hardware modules
  status_led_init();
  button_init(PICO_DEFAULT_BUTTON_PIN);
  button_set_callback(on_button);

  vk1s56e_init(16, 26, 17);  // nCS, nWR, DATA
  lcd_set_icons(LCD_ICON_PROG | LCD_ICON_ARROW);
  render();

  while (!stdio_usb_connected())
    sleep_ms(10);
  sleep_ms(200);

  while (true) {
    // Background tasks (non-blocking)
    button_task_update();

    // Small sleep or let it run free-spin with tight loop
    tight_loop_contents();
  }
}
