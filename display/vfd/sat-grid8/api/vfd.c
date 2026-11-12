#include "et16315.h"
#include "vfd.h"

//------------------------------------------------------------------------------

#define SEG_A   (1u << 0)
#define SEG_J   (1u << 1)
#define SEG_I   (1u << 2)
#define SEG_H   (1u << 3)
#define SEG_B   (1u << 4)
#define SEG_F   (1u << 5)
#define SEG_G2  (1u << 6)
#define SEG_DP  (1u << 7)
#define SEG_G1  (1u << 8)
#define SEG_C   (1u << 9)
#define SEG_E   (1u << 10)
#define SEG_M   (1u << 11)
#define SEG_L   (1u << 12)
#define SEG_K   (1u << 13)
#define SEG_D   (1u << 14)

#define SEG_G   (SEG_G1 | SEG_G2)

static const uint16_t s_font_digits[10] = {
  /* 0 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_J | SEG_M,
  /* 1 */ SEG_B | SEG_C,
  /* 2 */ SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,
  /* 3 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
  /* 4 */ SEG_F | SEG_G | SEG_B | SEG_C,
  /* 5 */ SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,
  /* 6 */ SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D,
  /* 7 */ SEG_A | SEG_B | SEG_C,
  /* 8 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
  /* 9 */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
};

static const uint16_t s_font_letters[26] = {
  /* A */ SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,
  /* B */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_I | SEG_L | SEG_G2,
  /* C */ SEG_A | SEG_D | SEG_E | SEG_F,
  /* D */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_I | SEG_L,
  /* E */ SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,
  /* F */ SEG_A | SEG_E | SEG_F | SEG_G1,
  /* G */ SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G2,
  /* H */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,
  /* I */ SEG_A | SEG_D | SEG_I | SEG_L,
  /* J */ SEG_B | SEG_C | SEG_D | SEG_E,
  /* K */ SEG_E | SEG_F | SEG_G1 | SEG_J | SEG_K,
  /* L */ SEG_D | SEG_E | SEG_F,
  /* M */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_H | SEG_J,
  /* N */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_H | SEG_K,
  /* O */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  /* P */ SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,
  /* Q */ SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_K,
  /* R */ SEG_A | SEG_B | SEG_E | SEG_F | SEG_G | SEG_K,
  /* S */ SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
  /* T */ SEG_A | SEG_I | SEG_L,
  /* U */ SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  /* V */ SEG_E | SEG_F | SEG_J | SEG_M,
  /* W */ SEG_B | SEG_C | SEG_E | SEG_F | SEG_K | SEG_M,
  /* X */ SEG_H | SEG_J | SEG_K | SEG_M,
  /* Y */ SEG_H | SEG_J | SEG_L,
  /* Z */ SEG_A | SEG_D | SEG_J | SEG_M,
};

//------------------------------------------------------------------------------

static uint16_t vfd_font_get(char c) {
  if (c >= '0' && c <= '9')
    return s_font_digits[c - '0'];
  if (c >= 'A' && c <= 'Z')
    return s_font_letters[c - 'A'];
  if (c == 'm')
    return SEG_C | SEG_E | SEG_G | SEG_L;
  if (c >= 'a' && c <= 'z')
    return s_font_letters[c - 'a'];

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
    case '-':
    default:   return SEG_G;
  }
}

//------------------------------------------------------------------------------

void vfd_init(uint8_t pin_stb, uint8_t pin_clk, uint8_t pin_dout, uint8_t pin_din) {
  et16315_init(pin_stb, pin_clk, pin_dout, pin_din);
  et16315_set_mode(ET16315_GRID_9);
  et16315_clear_block(0, VFD_GRIDS * 3);
  et16315_display(true, ET16315_DUTY_14_16);
}

void vfd_print(const char *text) {
  for (int i = 0; i < VFD_GRIDS - 1; i++) {
    char c = ' ';
    if (text && text[i] != '\0') {
      c = text[i];
    } else {
      text = NULL;                           // za koncom reťazca už len medzery
    }
    s_fb[VFD_DIGITS - 1 - i] = vfd_font_get(c);
  }

  for (int g = 0; g < VFD_DIGITS; g++)
    vfd_write_grid(g, s_fb[g]);
}
