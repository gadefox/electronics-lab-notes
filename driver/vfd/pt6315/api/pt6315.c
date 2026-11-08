#include "pico/stdlib.h"
#include "pt6315.h"

//------------------------------------------------------------------------------

// PT6315 command codes
#define PT6315_CMD_WRITE_RAM 0x40  // write display RAM, auto increment
#define PT6315_CMD_WRITE_LED 0x41  // write LED port
#define PT6315_CMD_READ_KEY  0x42  // read key data
#define PT6315_CMD_DISPLAY   0x80  // display control
#define PT6315_CMD_ADDR      0xC0  // address (| 0x00..0x23)

#define PT6315_DISPLAY_ON    (1 << 3)

#define PT6315_DELAY_US      1

//------------------------------------------------------------------------------

static uint8_t s_pin_stb;
static uint8_t s_pin_clk;
static uint8_t s_pin_din;
static uint8_t s_pin_dout;

//------------------------------------------------------------------------------

// LSB first, data latched on CLK rising edge
static void pt6315_send_byte(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    gpio_put(s_pin_clk, 0);

    gpio_put(s_pin_din, (data >> i) & 1);
    sleep_us(PT6315_DELAY_US);

    gpio_put(s_pin_clk, 1);
    sleep_us(PT6315_DELAY_US);
  }
}

// LSB first, data changes on CLK falling edge
static uint8_t pt6315_recv_byte(void) {
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) {
    gpio_put(s_pin_clk, 0);
    sleep_us(PT6315_DELAY_US);

    if (gpio_get(s_pin_dout))
      data |= (1 << i);

    gpio_put(s_pin_clk, 1);
    sleep_us(PT6315_DELAY_US);
  }
  return data;
}

static inline void pt6315_select(void) {
  gpio_put(s_pin_stb, 0);
  sleep_us(PT6315_DELAY_US);
}

static inline void pt6315_deselect(void) {
  gpio_put(s_pin_stb, 1);
  sleep_us(PT6315_DELAY_US);
}

// Single byte command in its own STB cycle
static void pt6315_send_cmd(uint8_t cmd) {
  pt6315_select();
  pt6315_send_byte(cmd);
  pt6315_deselect();
}

// Command 2 (write RAM) + command 3 (address), STB stays low for the data
static void pt6315_begin_write(uint8_t addr) {
  pt6315_send_cmd(PT6315_CMD_WRITE_RAM);
  pt6315_select();
  pt6315_send_byte(PT6315_CMD_ADDR | addr);
}

static inline void pt6315_gpio_init_out(uint8_t pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
}

static inline void pt6315_gpio_init_in(uint8_t pin, bool pull_up) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);

  if (pull_up)
    gpio_pull_up(pin);
}

//------------------------------------------------------------------------------

void pt6315_init(uint8_t pin_stb, uint8_t pin_clk, uint8_t pin_din,
                 uint8_t pin_dout) {
  s_pin_stb = pin_stb;
  pt6315_gpio_init_out(pin_stb);

  s_pin_clk = pin_clk;
  pt6315_gpio_init_out(pin_clk);

  s_pin_din = pin_din;
  pt6315_gpio_init_out(pin_din);

  s_pin_dout = pin_dout; // DOUT is open-drain, external pull-up 1k..10k recommended
  pt6315_gpio_init_in(pin_dout, true);
}

void pt6315_set_mode(pt6315_mode_t mode) {
  pt6315_send_cmd((uint8_t)mode);
}

void pt6315_display(bool on, pt6315_duty_t duty) {
  pt6315_send_cmd(PT6315_CMD_DISPLAY | (on ? PT6315_DISPLAY_ON : 0) | duty);
}

void pt6315_write(uint8_t addr, uint8_t data) {
  pt6315_begin_write(addr);
  pt6315_send_byte(data);
  pt6315_deselect();
}

void pt6315_write_block(uint8_t addr, const uint8_t *data, uint8_t length) {
  pt6315_begin_write(addr);

  for (uint8_t i = 0; i < length; i++)
    pt6315_send_byte(data[i]);

  pt6315_deselect();
}

void pt6315_clear_block(uint8_t addr, uint8_t length) {
  pt6315_begin_write(addr);

  for (uint8_t i = 0; i < length; i++)
    pt6315_send_byte(0);

  pt6315_deselect();
}

void pt6315_set_leds(uint8_t leds) {
  pt6315_select();
  pt6315_send_byte(PT6315_CMD_WRITE_LED);
  pt6315_send_byte(leds & 0x0F);
  pt6315_deselect();
}

void pt6315_read_keys(uint8_t keys[4]) {
  pt6315_select();
  pt6315_send_byte(PT6315_CMD_READ_KEY);
  sleep_us(1); // t_wait >= 1us

  for (uint8_t i = 0; i < 4; i++)
    keys[i] = pt6315_recv_byte();

  pt6315_deselect();
}

bool pt6315_key_pressed(const uint8_t keys[4], uint8_t ks_index,
                        uint8_t key_index) {
  uint8_t bit = ks_index * 2 + key_index;
  return keys[bit >> 3] & (1 << (bit & 7));
}
