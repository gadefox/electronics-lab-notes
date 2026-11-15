#include <string.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "lcd.h"

//------------------------------------------------------------------------------

#define LCD_ADDR  0x3C

#define LCD_COLS  128
#define LCD_ROWS  80

#define LCD_PAGES  (LCD_ROWS >> 3)

#define PIX_MASK(y)  ((uint8_t)(1 << ((y) & 0b111)))

//------------------------------------------------------------------------------

static const uint8_t s_init_seq[] = {
  0x3C, 0x0C, 0x17, 0x06,
  0x3D, 0x85, 0x11, 0x0C,
  0x3E, 0x0A, 0x06, 0x11,
  0x3F, 0x4A, 0xFF
};

static uint8_t s_frame_buf[LCD_PAGES][LCD_COLS];
static uint8_t s_pin_rst;

#if LCD_TEST
static const uint8_t SYM_TL[8] = { 0xFF,0x01,0x01,0x01,0x01,0x01,0x01,0x01 }; // ┌
static const uint8_t SYM_TR[8] = { 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xFF }; // ┐
static const uint8_t SYM_BL[8] = { 0xFF,0x80,0x80,0x80,0x80,0x80,0x80,0x80 }; // └
static const uint8_t SYM_BR[8] = { 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xFF }; // ┘
#endif // LCD_TEST

//------------------------------------------------------------------------------

static void lcd_send_cmd(uint8_t c) {
  uint8_t data[2] = { 0x80, c };
  i2c_write_blocking(i2c0, LCD_ADDR, data, sizeof(data), false);
}

static void fb_set_pixel(uint8_t x, uint8_t y, bool status) {
  if (x >= LCD_COLS || y >= LCD_ROWS)
    return;

  uint8_t *ptr = &s_frame_buf[y >> 3][x];

  if (status)
    *ptr |= PIX_MASK(y);
  else
    *ptr &= ~PIX_MASK(y);
}

//------------------------------------------------------------------------------

void lcd_init(uint8_t pin_sda, uint8_t pin_scl, uint8_t pin_rst) {
  i2c_init(i2c0, 400000); // 400kHz
  gpio_set_function(pin_sda, GPIO_FUNC_I2C);
  gpio_set_function(pin_scl, GPIO_FUNC_I2C);

  s_pin_rst = pin_rst;
  gpio_init(pin_rst);
  gpio_set_dir(pin_rst, GPIO_OUT);
}

void lcd_reset(void) {
  gpio_put(s_pin_rst, 0);
  sleep_us(3);
  gpio_put(s_pin_rst, 1);
  sleep_us(10);
}

void lcd_init_seq(void) {
  for (size_t i = 0; i < sizeof(s_init_seq); i++)
    lcd_send_cmd(s_init_seq[i]);

  sleep_us(100);
}

void lcd_clear() {
  memset(s_frame_buf, 0, sizeof(s_frame_buf));
  lcd_send_cmd(0x01);
}

void lcd_flush(void) {
  uint8_t data[LCD_COLS + 1];
  data[0] = 0x40; // Control byte: data stream

  for (int page = 0; page < LCD_PAGES; page++) {
    lcd_send_cmd(0x38);
    lcd_send_cmd(0x80);        // X = 0
    lcd_send_cmd(0x40 + page); // Y = page

    memcpy(&data[1], &s_frame_buf[page][0], LCD_COLS);
    i2c_write_blocking(i2c0, LCD_ADDR, data, sizeof(data), false);
  }
}

//------------------------------------------------------------------------------

#if LCD_TEST

static void fb_fill_grid(uint8_t x, uint8_t page, uint8_t data) {
  if (x > LCD_COLS - 8 || page >= LCD_PAGES)
    return;

  for (int i = 0; i < 8; i++)
    s_frame_buf[page][x + i] = data;
}

static void fb_fill_vlines(uint8_t x, uint8_t page) {
  if (x > LCD_COLS - 8 || page >= LCD_PAGES)
    return;

  for (int i = 0; i < 8; i++)
    s_frame_buf[page][x + i] = (i & 1) ? 0x00 : 0xFF;
}

void lcd_fb_draw_diamond(bool status) {
  for (int page = 0; page < LCD_PAGES; page++) {
    int d = (page < LCD_PAGES / 2) ? page : (LCD_PAGES - 1 - page);

    int num = d + 2;
    if (num > LCD_COLS / 16)
      num = LCD_COLS / 16;

    int width = (2 * num - 1) * 8;
    int x = (LCD_COLS - width) / 2;

    for (int i = 0; i < num; i++) {
      if (page % 2 == status)
        fb_fill_grid(x + i * 16, page, 0xAA);
      else
        fb_fill_vlines(x + i * 16, page);
    }
  }
}

static void fb_draw_sym(uint8_t x, uint8_t page, const uint8_t *sym) {
  if (x > LCD_COLS - 8 || page >= LCD_PAGES)
    return;

  memcpy(&s_frame_buf[page][x], sym, 8);
}

void lcd_fb_draw_corners(void) {
  fb_draw_sym(0,            0,             SYM_TL);
  fb_draw_sym(LCD_COLS - 8, 0,             SYM_TR);
  fb_draw_sym(0,            LCD_PAGES - 1, SYM_BL);
  fb_draw_sym(LCD_COLS - 8, LCD_PAGES - 1, SYM_BR);
}

#endif // LCD_TEST
