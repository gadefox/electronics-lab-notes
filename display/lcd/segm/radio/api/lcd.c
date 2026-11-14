#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"
#include "lcd.h"
#include "vk1s56e.h"

//------------------------------------------------------------------------------

#define LCD_SEGS  7
#define LCD_NUM   3

#define COM0  (1 << 0)
#define COM1  (1 << 1)
#define COM2  (1 << 2)
#define COM3  (1 << 3)

#define SEG_A  (1 << 0)
#define SEG_B  (1 << 1)
#define SEG_C  (1 << 2)
#define SEG_D  (1 << 3)
#define SEG_E  (1 << 4)
#define SEG_F  (1 << 5)
#define SEG_G  (1 << 6)

//------------------------------------------------------------------------------
// Font definition (bit0=a, bit1=b, bit2=c, bit3=d, bit4=e, bit5=f, bit6=g)

static const uint8_t s_digit_font[] = {
  /* 0 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  /* 1 */ SEG_B | SEG_C,
  /* 2 */ SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,
  /* 3 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
  /* 4 */ SEG_B | SEG_C | SEG_F | SEG_G,
  /* 5 */ SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
  /* 6 */ SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
  /* 7 */ SEG_A | SEG_B | SEG_C,
  /* 8 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
  /* 9 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G
};

//------------------------------------------------------------------------------

// RAM buffer
static uint8_t s_ram_buf[LCD_SEGS];

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
  uint8_t addr = (grid << 1) + 1; // 1,3,5
  lcd_set_pixel(addr, COM1, seg_mask & SEG_F);
  lcd_set_pixel(addr, COM2, seg_mask & SEG_G);
  lcd_set_pixel(addr, COM3, seg_mask & SEG_E);

  addr++; // 2,4,6
  lcd_set_pixel(addr, COM0, seg_mask & SEG_A);
  lcd_set_pixel(addr, COM1, seg_mask & SEG_B);
  lcd_set_pixel(addr, COM2, seg_mask & SEG_C);
  lcd_set_pixel(addr, COM3, seg_mask & SEG_D);
}

static bool lcd_format_num(int16_t value, uint8_t *grid) {
  memset(grid, 0, LCD_NUM);

  bool is_negative = value < 0;
  uint32_t abs_val = is_negative ? 0u - (uint32_t)value : (uint32_t)value;

  uint8_t buf[5];  // UINT16_MAX_DIGITS
  int count = 0;

  do {
    buf[count++] = abs_val % 10;
    abs_val /= 10;
  } while (abs_val);

  if (count + is_negative > LCD_NUM)
    goto overflow;

  for (int i = 0; i < count; i++) {
    uint8_t digit = buf[i];
    grid[LCD_NUM - 1 - i] = s_digit_font[digit];
  }

  if (is_negative)
    grid[LCD_NUM - 1 - count] = SEG_G;

  return true;

overflow:
  memset(grid, SEG_G, LCD_NUM);
  return false;
}

//------------------------------------------------------------------------------

void lcd_set_icons(uint32_t icon_mask) {
  /* COM0 */
  lcd_set_pixel(0, COM0, icon_mask & LCD_ICON_CARD);
  lcd_set_pixel(1, COM0, icon_mask & LCD_ICON_ALL);
  lcd_set_pixel(3, COM0, icon_mask & LCD_ICON_ARROW);

  /* COM1 */
  lcd_set_pixel(0, COM1, icon_mask & LCD_ICON_RAND);

  /* COM2 */
  lcd_set_pixel(0, COM2, icon_mask & LCD_ICON_REP);

  /* COM3 */
  lcd_set_pixel(0, COM3, icon_mask & LCD_ICON_PROG);
}

void lcd_draw_num(int16_t value) {
  uint8_t grid[LCD_NUM];
  lcd_format_num(value, grid);

  for (int i = 0; i < LCD_NUM; i++)
    lcd_draw_grid(i, grid[i]);
}

void lcd_flush(void) {
  uint8_t vk_ram[LCD_SEGS * 2 - 1] = {0};
  for (int n = 0; n < LCD_SEGS; n++)
    vk_ram[n * 2] = s_ram_buf[n];

  vk1s56e_write_block(0, vk_ram, sizeof(vk_ram));
}
