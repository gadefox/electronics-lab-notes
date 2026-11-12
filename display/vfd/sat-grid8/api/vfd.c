#include <string.h>
#include "et16315.h"
#include "vfd.h"

//------------------------------------------------------------------------------

#define VFD_GRIDS   9
#define VFD_DIGITS  5

#define LOBYTE(x)  ((uint8_t)((x) & 0xFF))
#define HIBYTE(x)  ((uint8_t)(((x) >> 8) & 0xFF))

//------------------------------------------------------------------------------

#define SEG_A   (1 << 0)
#define SEG_J   (1 << 1)
#define SEG_I   (1 << 2)
#define SEG_H   (1 << 3)
#define SEG_B   (1 << 4)
#define SEG_F   (1 << 5)
#define SEG_G2  (1 << 6)
#define SEG_DP  (1 << 7)
#define SEG_G1  (1 << 8)
#define SEG_C   (1 << 9)
#define SEG_E   (1 << 10)
#define SEG_M   (1 << 11)
#define SEG_L   (1 << 12)
#define SEG_K   (1 << 13)
#define SEG_D   (1 << 14)

#define SEG_G  (SEG_G1 | SEG_G2)

//------------------------------------------------------------------------------

static const uint16_t s_font_digits[10] = {
  /* 0 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_J | SEG_M,
  /* 1 */ SEG_B | SEG_C,
  /* 2 */ SEG_A | SEG_B | SEG_G | SEG_E | SEG_D | SEG_DP,
  /* 3 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_G | SEG_DP,
  /* 4 */ SEG_F | SEG_G | SEG_B | SEG_C | SEG_DP,
  /* 5 */ SEG_A | SEG_F | SEG_G | SEG_C | SEG_D | SEG_DP,
  /* 6 */ SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D | SEG_DP,
  /* 7 */ SEG_A | SEG_B | SEG_C,
  /* 8 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G | SEG_DP,
  /* 9 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G | SEG_DP
};

static const uint16_t s_font_alpha[26] = {
  /* A */ SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G | SEG_DP,
  /* B */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_I | SEG_L | SEG_G2,
  /* C */ SEG_A | SEG_D | SEG_E | SEG_F,
  /* D */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_I | SEG_L,
  /* E */ SEG_A | SEG_D | SEG_E | SEG_F | SEG_G | SEG_DP,
  /* F */ SEG_A | SEG_E | SEG_F | SEG_G1 | SEG_DP,
  /* G */ SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G2,
  /* H */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_G | SEG_DP,
  /* I */ SEG_A | SEG_D | SEG_I | SEG_L,
  /* J */ SEG_B | SEG_C | SEG_D | SEG_E,
  /* K */ SEG_E | SEG_F | SEG_G1 | SEG_J | SEG_K | SEG_DP,
  /* L */ SEG_D | SEG_E | SEG_F,
  /* M */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_H | SEG_J,
  /* N */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_H | SEG_K,
  /* O */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  /* P */ SEG_A | SEG_B | SEG_E | SEG_F | SEG_G | SEG_DP,
  /* Q */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_K,
  /* R */ SEG_A | SEG_B | SEG_E | SEG_F | SEG_G | SEG_K | SEG_DP,
  /* S */ SEG_A | SEG_C | SEG_D | SEG_F | SEG_G | SEG_DP,
  /* T */ SEG_A | SEG_I | SEG_L,
  /* U */ SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  /* V */ SEG_E | SEG_F | SEG_J | SEG_M,
  /* W */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_K | SEG_M | SEG_DP,
  /* X */ SEG_H | SEG_J | SEG_K | SEG_M,
  /* Y */ SEG_H | SEG_J | SEG_L,
  /* Z */ SEG_A | SEG_D | SEG_J | SEG_M
};

// RAM buffer
static uint8_t s_ram_buf[VFD_GRIDS][3];

//------------------------------------------------------------------------------

static uint16_t vfd_font_get(char c) {
  if (c >= '0' && c <= '9')
    return s_font_digits[c - '0'];
  if (c >= 'A' && c <= 'Z')
    return s_font_alpha[c - 'A'];
  if (c == 'm')
    return SEG_C | SEG_E | SEG_G | SEG_L;
  if (c >= 'a' && c <= 'z')
    return s_font_alpha[c - 'a'];

  switch (c) {
    case '+':  return SEG_G | SEG_I | SEG_L;
    case '*':  return SEG_G | SEG_I | SEG_L | SEG_H | SEG_J | SEG_K | SEG_M;
    case '/':  return SEG_J | SEG_M;
    case '\\': return SEG_H | SEG_K;
    case '_':  return SEG_D;
    case '=':  return SEG_G | SEG_D;
    case '<':  return SEG_J | SEG_K;
    case '>':  return SEG_H | SEG_M;
    case '.':  return SEG_DP;
    case '-':  return SEG_G;
    default:   return 0;
  }
}

static bool vfd_format_num(int16_t value, uint8_t decimal_pos, uint16_t *grid)
{
  if (decimal_pos >= VFD_DIGITS - 1)
    goto overflow;

  memset(grid, 0, VFD_DIGITS * sizeof(uint16_t));

  // Remember the sign, then take the magnitude using unsigned arithmetic.
  // Negating in unsigned (0u - x) is well-defined modulo 2^32, so it is
  // safe even for INT16_MIN (-32768 -> 32768) with no signed-overflow UB.
  bool is_negative = value < 0;
  uint32_t abs_val = is_negative ? 0u - (uint32_t)value : (uint32_t)value;

  // Split the magnitude into decimal digits, least significant first:
  // buf[0] = ones, buf[1] = tens..
  uint8_t buf[5];  // UINT16_MAX_DIGITS
  int count = 0;

  do {
    buf[count++] = abs_val % 10;
    abs_val /= 10;
  } while (abs_val);

  // Pad with leading zeros so there is at least one digit to the left of
  // the decimal point (e.g. value 5 with decimal_pos 1 -> "0.5", not ".5").
  while (count < decimal_pos + 1)
    buf[count++] = 0;

  // Count how many display cells the result needs: one per digit, plus one
  // for the decimal point (it has its own grid), plus one for the minus sign.
  int cells = count + (decimal_pos > 0 ? 1 : 0) + is_negative;
  if (cells > VFD_DIGITS)
    goto overflow;

  // Fill the grid right-to-left.
  int pos = VFD_DIGITS - 1;

  for (int i = 0; i < count; i++) {
    if (decimal_pos > 0 && i == decimal_pos)
      grid[pos--] = SEG_DP;

    grid[pos--] = s_font_digits[buf[i]];
  }

  // Add minus sign if necessary
  if (is_negative)
    grid[pos] = SEG_G;

  return true;

overflow:
  for (int i = 0; i < VFD_DIGITS; i++)
    grid[i] = SEG_G;

  return false;
}

//------------------------------------------------------------------------------

void vfd_init(uint8_t pin_stb, uint8_t pin_clk, uint8_t pin_dout, uint8_t pin_din) {
  et16315_init(pin_stb, pin_clk, pin_dout, pin_din);
  et16315_set_mode(ET16315_GRID_9);
  et16315_clear_block(0, sizeof(s_ram_buf));
  et16315_display(true, ET16315_DUTY_14_16);
}

void vfd_draw_text(const char *text, uint8_t grid_pos) {
  int grid_idx = (VFD_GRIDS - 2) - grid_pos;  // pos 0 -> grid 7, pos 7 -> grid 0

  while (*text && grid_idx >= 0) {
    uint16_t seg = vfd_font_get(*text);

    s_ram_buf[grid_idx][0] = LOBYTE(seg);
    s_ram_buf[grid_idx][1] = HIBYTE(seg);

    text++;
    grid_idx--;
  }
}

void vfd_draw_num(int16_t value, uint8_t decimal_pos) {
  uint16_t grid[VFD_DIGITS];
  vfd_format_num(value, decimal_pos, grid);

  int grid_idx = VFD_GRIDS - 2;
  for (size_t i = 0; i < VFD_DIGITS; i++) {
    s_ram_buf[grid_idx][0] = LOBYTE(grid[i]);
    s_ram_buf[grid_idx][1] = HIBYTE(grid[i]);

    grid_idx--;
  }
}

void vfd_flush(void) {
  et16315_write_block(0, (uint8_t*)s_ram_buf, sizeof(s_ram_buf));
}
