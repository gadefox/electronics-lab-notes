#ifndef LCD_HT1621_BLUE_H
#define LCD_HT1621_BLUE_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_ICON_IN       (1 << 0)
#define LCD_ICON_OUT      (1 << 1)
#define LCD_ICON_SET      (1 << 2)
#define LCD_ICON_TEMP_C   (1 << 3)
#define LCD_ICON_V        (1 << 4)
#define LCD_ICON_W        (1 << 5)
#define LCD_ICON_PERCENT  (1 << 6)
#define LCD_ICON_A        (1 << 7)
#define LCD_ICON_H        (1 << 8)
#define LCD_ICON_COLON    (1 << 9)

#define LCD_ICON_MAX  10

typedef enum {
  ROW_TOP,
  ROW_BOTTOM
} lcd_row_t;

// Icon control
void lcd_set_icons(uint32_t icon_mask);

// Text & Numbers
void lcd_draw_text(lcd_row_t row, const char* text);
void lcd_draw_num(lcd_row_t row, int16_t value, uint8_t decimal_pos);

// Blinking & Editing features
void lcd_blink_row(lcd_row_t row, bool enable);
void lcd_blink_pos(uint8_t position, bool enable);

// Periodic background task (handles blinking animation)
void lcd_task_update(void);

// Push the display RAM shadow buffer to the HT1621
void lcd_flush(void);

#endif // LCD_HT1621_BLUE_H
