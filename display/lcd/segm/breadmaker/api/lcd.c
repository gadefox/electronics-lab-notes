#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"
#include "lcd.h"

//------------------------------------------------------------------------------

#define COM0 (1u << 0)
#define COM1 (1u << 1)
#define COM2 (1u << 2)
#define COM3 (1u << 3)

#define SEG_A (1u << 0)
#define SEG_B (1u << 1)
#define SEG_C (1u << 2)
#define SEG_D (1u << 3)
#define SEG_E (1u << 4)
#define SEG_F (1u << 5)
#define SEG_G (1u << 6)

// Font definition (bit0=a, bit1=b, bit2=c, bit3=d, bit4=e, bit5=f, bit6=g)
#define GR_BLANK 0x00
#define GR_MINUS 0x40

static const uint8_t s_digit_font[] = {
  /* 0 */ 0x3F,
  /* 1 */ 0x06,
  /* 2 */ 0x5B,
  /* 3 */ 0x4F,
  /* 4 */ 0x66,
  /* 5 */ 0x6D,
  /* 6 */ 0x7D,
  /* 7 */ 0x07,
  /* 8 */ 0x7F,
  /* 9 */ 0x6F
};

// 9 RAM addresses
static uint8_t s_ram_buf[9];

//------------------------------------------------------------------------------
// Helper to modify specific bit (COM) on a specific RAM address
static void lcd_set_pixel(uint8_t addr, uint8_t com_mask, bool state) {
  if (state)
    s_ram_buf[addr] |= com_mask;
  else
    s_ram_buf[addr] &= ~com_mask;
}

// Helper to draw a 7-segment grid
static void lcd_draw_grid(uint8_t grid, uint8_t seg_mask) {
  uint8_t addr = grid << 1; // 0,2,4,6
  lcd_set_pixel(addr, COM0, seg_mask & SEG_F);
  lcd_set_pixel(addr, COM1, seg_mask & SEG_G);
  lcd_set_pixel(addr, COM2, seg_mask & SEG_E);
  lcd_set_pixel(addr, COM3, seg_mask & SEG_D);

  addr++; // 1,3,5,7
  lcd_set_pixel(addr, COM0, seg_mask & SEG_A);
  lcd_set_pixel(addr, COM1, seg_mask & SEG_B);
  lcd_set_pixel(addr, COM2, seg_mask & SEG_C);
}

//------------------------------------------------------------------------------

void lcd_set_icons(uint32_t icon_mask) {
  lcd_set_pixel(1, COM3, icon_mask & LCD_ICON_ONE0);
  lcd_set_pixel(3, COM3, icon_mask & LCD_ICON_ONE1);
  lcd_set_pixel(5, COM3, icon_mask & LCD_ICON_COLON);
  lcd_set_pixel(7, COM3, icon_mask & LCD_ICON_TRI4);

  lcd_set_pixel(8, COM0, icon_mask & LCD_ICON_TRI0);
  lcd_set_pixel(8, COM1, icon_mask & LCD_ICON_TRI1);
  lcd_set_pixel(8, COM2, icon_mask & LCD_ICON_TRI2);
  lcd_set_pixel(8, COM3, icon_mask & LCD_ICON_TRI3);
}

void lcd_flush(void) {
}
