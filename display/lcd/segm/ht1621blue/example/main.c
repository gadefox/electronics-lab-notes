#include "pico/status_led.h"
#include "pico/stdlib.h"
#include "button.h"
#include "ht1621.h"
#include "lcd.h"

static uint8_t s_state;

#if 0
static void render(void) {
  status_led_set_state(!(s_state % 2));

  switch (s_state) {
    case 0:
      lcd_set_icons(LCD_ICON_SET | LCD_ICON_V);
      lcd_draw_num(ROW_TOP, 370, 1);
      lcd_draw_text(ROW_BOTTOM, "OUP");
      break;

    case 1:
      lcd_set_icons(LCD_ICON_SET | LCD_ICON_A);
      lcd_draw_text(ROW_TOP, "OCP");
      lcd_draw_num(ROW_BOTTOM, 160, 2);
      break;

    case 2:
      lcd_set_icons(LCD_ICON_SET | LCD_ICON_H | LCD_ICON_COLON);
      lcd_draw_text(ROW_TOP, "OHP");
      lcd_draw_num(ROW_BOTTOM, 9959, 0);
      break;

    case 3:
      lcd_set_icons(LCD_ICON_SET | LCD_ICON_TEMP_C);
      lcd_draw_num(ROW_TOP, 80, 0);
      lcd_draw_text(ROW_BOTTOM, "OHC");
      break;

    case 4:
      lcd_set_icons(LCD_ICON_SET | LCD_ICON_W);
      lcd_draw_text(ROW_TOP, "OPP");
      lcd_draw_num(ROW_BOTTOM, 8201, 2);
      break;

    case 5:
      lcd_set_icons(LCD_ICON_SET);
      lcd_draw_text(ROW_TOP, "OAP");
      lcd_draw_text(ROW_BOTTOM, "OFF");
      break;

    case 6:
      lcd_set_icons(LCD_ICON_SET | LCD_ICON_A | LCD_ICON_H);
      lcd_draw_text(ROW_TOP, "OAP");
      lcd_draw_num(ROW_BOTTOM, 11, 2);
      break;

    case 7:
      lcd_set_icons(LCD_ICON_SET);
      lcd_draw_text(ROW_TOP, "TEST");
      lcd_draw_text(ROW_BOTTOM, "----");
      lcd_blink_row(ROW_BOTTOM, true);
      break;
  }

  // Push everything to HT1621 hardware at once
  lcd_flush();
  s_state = (s_state + 1) % 8;
}
#endif

static int s_num = 1343;

static void on_button(void) {
  status_led_set_state(!(s_state % 2));

  if (s_state < LCD_ICON_MAX) {
    lcd_set_icons(1 << s_state);
  } else if (s_state == LCD_ICON_MAX) {
    lcd_set_icons(LCD_ICON_IN | LCD_ICON_A);
    lcd_draw_text(ROW_TOP, "CAL");
    lcd_draw_num(ROW_BOTTOM, s_num, 2);
    lcd_blink_pos(6, true);
  } else {
    s_num -= 10;
    lcd_draw_num(ROW_BOTTOM, s_num, 2);
  }

  lcd_flush();
  s_state++;
}

int main() {
  stdio_init_all();

  // Initialize hardware modules
  status_led_init();
  button_init(PICO_DEFAULT_BUTTON_PIN);
  button_set_callback(on_button);

  while (true) {
    // Background tasks (non-blocking)
    button_task_update();
    lcd_task_update();

    // Small sleep or let it run free-spin with tight loop
    tight_loop_contents();
  }
}
