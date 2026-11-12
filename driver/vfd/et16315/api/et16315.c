#include "pico/stdlib.h"
#include "et16315.h"

//------------------------------------------------------------------------------

// ET16315 command codes
#define ET16315_CMD_WRITE_RAM 0x40  // write display RAM, auto increment
#define ET16315_CMD_WRITE_LED 0x41  // write LED port
#define ET16315_CMD_READ_KEY  0x42  // read key data
#define ET16315_CMD_DISPLAY   0x80  // display control
#define ET16315_CMD_ADDR      0xC0  // address (| 0x00..0x23)

#define ET16315_DISPLAY_ON    (1 << 3)

#define ET16315_DELAY_US      50

//------------------------------------------------------------------------------

static uint8_t s_pin_stb;
static uint8_t s_pin_clk;
static uint8_t s_pin_din;
static uint8_t s_pin_dout;

//------------------------------------------------------------------------------

// LSB first, data latched on CLK rising edge
static void et16315_send_byte(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    gpio_put(s_pin_clk, 0);

    gpio_put(s_pin_din, (data >> i) & 1);
    sleep_us(ET16315_DELAY_US);

    gpio_put(s_pin_clk, 1);
    sleep_us(ET16315_DELAY_US);
  }
}

// LSB first, data changes on CLK falling edge
static uint8_t et16315_recv_byte(void) {
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) {
    gpio_put(s_pin_clk, 0);
    sleep_us(ET16315_DELAY_US);

    if (gpio_get(s_pin_dout))
      data |= (1 << i);

    gpio_put(s_pin_clk, 1);
    sleep_us(ET16315_DELAY_US);
  }
  return data;
}

static inline void et16315_select(void) {
  gpio_put(s_pin_stb, 0);
  sleep_us(ET16315_DELAY_US);
}

static inline void et16315_deselect(void) {
  gpio_put(s_pin_stb, 1);
  sleep_us(ET16315_DELAY_US);
}

// Single byte command in its own STB cycle
static void et16315_send_cmd(uint8_t cmd) {
  et16315_select();
  et16315_send_byte(cmd);
  et16315_deselect();
}

// Command 2 (write RAM) + command 3 (address), STB stays low for the data
static void et16315_begin_write(uint8_t addr) {
  et16315_send_cmd(ET16315_CMD_WRITE_RAM);
  et16315_select();
  et16315_send_byte(ET16315_CMD_ADDR | addr);
}

static inline void et16315_gpio_init_out(uint8_t pin) {
  gpio_init(pin);
  gpio_put(pin, 1);
  gpio_set_dir(pin, GPIO_OUT);
}

//------------------------------------------------------------------------------

void et16315_init(uint8_t pin_stb, uint8_t pin_clk, uint8_t pin_dout, uint8_t pin_din) {
  s_pin_stb = pin_stb;
  et16315_gpio_init_out(pin_stb);

  s_pin_clk = pin_clk;
  et16315_gpio_init_out(pin_clk);

  s_pin_din = pin_din;
  et16315_gpio_init_out(pin_din);

  s_pin_dout = pin_dout; // DOUT is open-drain, external pull-up 1k..10k recommended
  gpio_init(pin_dout);
  gpio_set_dir(pin_dout, GPIO_IN);
  gpio_pull_up(pin_dout);
}

void et16315_set_mode(et16315_mode_t mode) {
  et16315_send_cmd((uint8_t)mode);
}

void et16315_display(bool on, et16315_duty_t duty) {
  et16315_send_cmd(ET16315_CMD_DISPLAY | (on ? ET16315_DISPLAY_ON : 0) | duty);
}

void et16315_write(uint8_t addr, uint8_t data) {
  et16315_begin_write(addr);
  et16315_send_byte(data);
  et16315_deselect();
}

void et16315_write_block(uint8_t addr, const uint8_t *data, uint8_t length) {
  et16315_begin_write(addr);

  for (uint8_t i = 0; i < length; i++)
    et16315_send_byte(data[i]);

  et16315_deselect();
}

void et16315_clear_block(uint8_t addr, uint8_t length) {
  et16315_begin_write(addr);

  for (uint8_t i = 0; i < length; i++)
    et16315_send_byte(0);

  et16315_deselect();
}

void et16315_set_leds(uint8_t leds) {
  et16315_select();
  et16315_send_byte(ET16315_CMD_WRITE_LED);
  et16315_send_byte(~leds & 0x0F);
  et16315_deselect();
}

void et16315_read_keys(uint8_t keys[4]) {
  et16315_select();
  et16315_send_byte(ET16315_CMD_READ_KEY);
  sleep_us(1); // t_wait >= 1us

  for (uint8_t i = 0; i < 4; i++)
    keys[i] = et16315_recv_byte();

  et16315_deselect();
}

bool et16315_key_pressed(const uint8_t keys[4], uint8_t ks_index, uint8_t key_index) {
  uint8_t bit = ks_index * 2 + key_index;
  return keys[bit >> 3] & (1 << (bit & 7));
}
