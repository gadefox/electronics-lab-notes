#ifndef LCD_RADIO_H
#define LCD_RADIO_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_ICON_CARD   (1 << 0)
#define LCD_ICON_RAND   (1 << 1)
#define LCD_ICON_REP    (1 << 2)
#define LCD_ICON_PROG   (1 << 3)
#define LCD_ICON_ALL    (1 << 4)
#define LCD_ICON_ARROW  (1 << 5)

#define LCD_ICON_MAX  6

// Icon control
void lcd_set_icons(uint32_t icon_mask);

// Numbers
void lcd_draw_num(int16_t value);

// Push the display RAM shadow buffer
void lcd_flush(void);

#endif // LCD_RADIO_H
