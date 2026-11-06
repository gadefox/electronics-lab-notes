#ifndef LCD_GLUCOMETER_H
#define LCD_GLUCOMETER_H

#include <stdbool.h>
#include <stdint.h>

#define LCD_TEST 1

void lcd_init(uint8_t pin_sda, uint8_t pin_scl, uint8_t pin_rst);
void lcd_reset(void);
void lcd_init_seq(void);
void lcd_clear(void);
void lcd_flush(void);

#if LCD_TEST
void lcd_fb_draw_diamond(bool status);
void lcd_fb_draw_corners(void);
#endif

#endif // LCD_GLUCOMETER_H
