#ifndef LCD_BREAD_MAKER_H
#define LCD_BREAD_MAKER_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_ICON_TRI0   (1 << 0)
#define LCD_ICON_TRI1   (1 << 1)
#define LCD_ICON_TRI2   (1 << 2)
#define LCD_ICON_TRI3   (1 << 3)
#define LCD_ICON_TRI4   (1 << 4)
#define LCD_ICON_ONE0   (1 << 5)
#define LCD_ICON_ONE1   (1 << 6)
#define LCD_ICON_COLON  (1 << 7)

#define LCD_ICON_MAX  8

// Icon control
void lcd_set_icons(uint32_t icon_mask);

// Numbers
void lcd_draw_num(lcd_row_t row, int16_t value);

// Push the display RAM shadow buffer
void lcd_flush(void);

#endif // LCD_BREAD_MAKER_H
