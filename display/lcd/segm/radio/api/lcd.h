#ifndef LCD_BREAD_MAKER_H
#define LCD_BREAD_MAKER_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_ICON_ALBUM  (1 << 0)
#define LCD_ICON_ALL    (1 << 1)
#define LCD_ICON_ARROW  (1 << 2)
#define LCD_ICON_CARD   (1 << 3)
#define LCD_ICON_MP3    (1 << 4)
#define LCD_ICON_PROG   (1 << 5)
#define LCD_ICON_RAND   (1 << 6)
#define LCD_ICON_REP    (1 << 7)
#define LCD_ICON_USB    (1 << 8)
#define LCD_ICON_WMA    (1 << 9)

#define LCD_ICON_MAX  10

// Icon control
void lcd_set_icons(uint32_t icon_mask);

// Numbers
void lcd_draw_num(lcd_row_t row, int16_t value);

// Push the display RAM shadow buffer
void lcd_flush(void);

#endif // LCD_BREAD_MAKER_H
