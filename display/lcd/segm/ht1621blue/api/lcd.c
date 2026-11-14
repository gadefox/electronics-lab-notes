#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"
#include "ht1621.h"
#include "lcd.h"

//------------------------------------------------------------------------------

#define LCD_DIGITS  4

#define BLINK_HALF_PERIOD_US  500000u

//------------------------------------------------------------------------------

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

static const uint8_t s_alpha_font[] = {
  /* A */ SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,
  /* B */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
  /* C */ SEG_A | SEG_D | SEG_E | SEG_F,
  /* D */ SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,                    // 'd'
  /* E */ SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,
  /* F */ SEG_A | SEG_E | SEG_F | SEG_G,
  /* G */ SEG_A | SEG_C | SEG_D | SEG_E | SEG_F,
  /* H */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,
  /* I */ SEG_E | SEG_F,
  /* J */ SEG_B | SEG_C | SEG_D | SEG_E,
  /* K */ 0,
  /* L */ SEG_D | SEG_E | SEG_F,
  /* M */ 0,
  /* N */ 0,
  /* O */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  /* P */ SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,
  /* Q */ SEG_A | SEG_B | SEG_C | SEG_F | SEG_G,                    // 'q'
  /* R */ SEG_E | SEG_G,                                            // 'r'
  /* S */ SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
  /* T */ SEG_D | SEG_E | SEG_F | SEG_G,                            // 't'
  /* U */ SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  /* V */ 0,
  /* W */ 0,
  /* X */ 0,
  /* Y */ SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
  /* Z */ SEG_A | SEG_B | SEG_D | SEG_E | SEG_G
};

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
// Map grid index (0 to 7) to physical address

static const uint8_t s_grid_pairs[8][2] = {
  /* top row */
  { 16, 15 }, // GR0
  { 14, 13 }, // GR1
  { 12, 11 }, // GR2
  { 10, 9 },  // GR3
  /* bottom row */
  { 0, 1 },   // GR4
  { 2, 3 },   // GR5
  { 4, 5 },   // GR6
  { 6, 7 }    // GR7
};

//------------------------------------------------------------------------------

// RAM buffer
static uint8_t s_ram_buf[18];

static uint8_t s_blink_mask; // Bitmask for blinking grid
static uint32_t s_blink_last;
static bool s_blink_phase;

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
  uint8_t addr0 = s_grid_pairs[grid][0];
  lcd_set_pixel(addr0, COM0, seg_mask & SEG_A);
  lcd_set_pixel(addr0, COM1, seg_mask & SEG_F);
  lcd_set_pixel(addr0, COM2, seg_mask & SEG_E);
  lcd_set_pixel(addr0, COM3, seg_mask & SEG_D);

  uint8_t addr1 = s_grid_pairs[grid][1];
  lcd_set_pixel(addr1, COM0, seg_mask & SEG_B);
  lcd_set_pixel(addr1, COM1, seg_mask & SEG_G);
  lcd_set_pixel(addr1, COM2, seg_mask & SEG_C);
}

// Helper to control decimal dots (DOT0-5)
static void lcd_set_dot(uint8_t dot, bool state) {
  int index = (dot < 3) ? dot : dot + 1;
  lcd_set_pixel(s_grid_pairs[index][1], COM3, state);
}

static bool lcd_format_num(int16_t value, uint8_t decimal_pos, uint8_t *grid) {
  if (decimal_pos >= LCD_DIGITS)
    goto overflow;

  memset(grid, 0, LCD_DIGITS);

  bool is_negative = value < 0;
  uint32_t abs_val = is_negative ? 0u - (uint32_t)value : (uint32_t)value;

  uint8_t buf[5];  // UINT16_MAX_DIGITS
  int count = 0;

  do {
    buf[count++] = abs_val % 10;
    abs_val /= 10;
  } while (abs_val);

  while (count < decimal_pos + 1)
    buf[count++] = 0;

  if (count + is_negative > LCD_DIGITS)
    goto overflow;

  for (int i = 0; i < count; i++) {
    uint8_t digit = buf[i];
    grid[LCD_DIGITS - 1 - i] = s_digit_font[digit];
  }

  if (is_negative)
    grid[LCD_DIGITS - 1 - count] = SEG_G;

  return true;

overflow:
  memset(grid, SEG_G, LCD_DIGITS);
  return false;
}

//------------------------------------------------------------------------------

void lcd_set_icons(uint32_t icon_mask) {
  /* addr7 */
  lcd_set_pixel(7, COM3, icon_mask & LCD_ICON_COLON);

  /* addr8 */
  lcd_set_pixel(8,  COM0, icon_mask & LCD_ICON_W);
  lcd_set_pixel(8,  COM1, icon_mask & LCD_ICON_PERCENT);
  lcd_set_pixel(8,  COM2, icon_mask & LCD_ICON_H);
  lcd_set_pixel(8,  COM3, icon_mask & LCD_ICON_A);

  /* addr9 */
  lcd_set_pixel(9,  COM3, icon_mask & LCD_ICON_V);

  /* addr17 */
  lcd_set_pixel(17, COM0, icon_mask & LCD_ICON_IN);
  lcd_set_pixel(17, COM1, icon_mask & LCD_ICON_OUT);
  lcd_set_pixel(17, COM2, icon_mask & LCD_ICON_SET);
  lcd_set_pixel(17, COM3, icon_mask & LCD_ICON_TEMP_C);
}

void lcd_draw_text(lcd_row_t row, const char* text) {
  int grid_idx = (row == ROW_TOP) ? 0 : LCD_DIGITS; // top: GR0-3, bottom: GR4-7
  int i = 0;

  for (i = 0; i < LCD_DIGITS; i++) {
    char c = toupper((unsigned char)text[i]);
    if ( c == '\0')
      break;

    uint8_t grid;
    if (c >= 'A' && c <= 'Z')
      grid = s_alpha_font[c - 'A'];
    else if (c >= '0' && c <= '9')
      grid = s_digit_font[c - '0'];
    else if (c == '-')
      grid = SEG_G;
    else
      grid = 0;

    lcd_draw_grid(grid_idx + i, grid);
  }

  for (; i < LCD_DIGITS; i++)
    lcd_draw_grid(grid_idx + i, 0);

  // Hide all dots in row
  int dot_idx = (row == ROW_TOP) ? 0 : 3; // top: DOT0-2, bottom: DOT3-5

  for (i = 0; i < 3; i++)
    lcd_set_dot(dot_idx + i, false);
}

void lcd_draw_num(lcd_row_t row, int16_t value, uint8_t decimal_pos) {
  uint8_t grid[LCD_DIGITS];
  lcd_format_num(value, decimal_pos, grid);

  int grid_idx = (row == ROW_TOP) ? 0 : LCD_DIGITS; // top: GR0-3, bottom: GR4-7

  for (size_t i = 0; i < sizeof(grid); i++)
    lcd_draw_grid(grid_idx + i, grid[i]);

  // decimal_pos: 0 = none, 1 = after pos 3, 2 = after pos 2, etc. (from right)
  int dot_idx = (row == ROW_TOP) ? 0 : 3; // top: DOT0-2, bottom: DOT3-5

  for (int i = 0; i < 3; i++) {
    bool has_dot = (decimal_pos == 3 - i);
    lcd_set_dot(dot_idx + i, has_dot);
  }
}

static void lcd_blink_mask_update(uint8_t new_mask) {
  if (!s_blink_mask && new_mask) {
    s_blink_last = time_us_32();
    s_blink_phase = true;
  }

  s_blink_mask = new_mask;
}

void lcd_blink_row(lcd_row_t row, bool enable) {
  uint8_t row_mask = (row == ROW_TOP) ? 0x0F : 0xF0;
  uint8_t new_mask = enable ? (s_blink_mask | row_mask) : (s_blink_mask & ~row_mask);
  lcd_blink_mask_update(new_mask);
}

void lcd_blink_pos(uint8_t position, bool enable) {
  if (position < 8) {
    uint8_t mask = enable ? (1 << position) : 0;
    lcd_blink_mask_update(mask);
  }
}

void lcd_task_update(void) {
  // Optimization: If no digits are set to blink, skip timing logic
  if (s_blink_mask) {
    uint32_t now = time_us_32();
    if (now - s_blink_last >= BLINK_HALF_PERIOD_US) {
      s_blink_last = now;
      s_blink_phase ^= 1;
      lcd_flush();
    }
  } else if (s_blink_phase) {
    // Ensure display state is fully visible if blinking was just disabled
    s_blink_phase = false;
    lcd_flush();
  }
}

void lcd_flush(void) {
  uint8_t temp_ram[sizeof(s_ram_buf)];
  memcpy(temp_ram, s_ram_buf, sizeof(s_ram_buf));

  if (s_blink_mask && s_blink_phase) {
    for (int i = 0; i < 8; i++) {
      if (s_blink_mask & (1 << i)) {
        uint8_t addr0 = s_grid_pairs[i][0];
        temp_ram[addr0] = 0;

        uint8_t addr1 = s_grid_pairs[i][1];
        temp_ram[addr1] &= COM3; // Do not hide dots and icons
      }
    }
  }

  ht1621_write_block(1, temp_ram, sizeof(temp_ram));
}
