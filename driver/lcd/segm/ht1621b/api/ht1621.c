#include "pico/stdlib.h"
#include "ht1621.h"

//------------------------------------------------------------------------------

// HT1621 command codes
#define HT1621_CMD_SYS_EN  0x01 // system enable
#define HT1621_CMD_LCD_ON  0x03 // LCD on
#define HT1621_CMD_BIAS    0x29 // 1/3 bias, 1/4 duty (COM0-3)

#define HT1621_ID_CMD    0x04 // 0b100
#define HT1621_ID_WRITE  0x05 // 0b101

// WR half-period in microseconds.
// Datasheet (HT1621 rev 3.40): min WR pulse width in write mode is 3.34 us @
// VDD=3V (1.67 us @ 5V), max WR clock is 150 kHz @ 3V. 4 us per phase -> 8 us
// period -> 125 kHz, which satisfies the 3V limits and therefore the 5V ones.
#define HT1621_DELAY_US  4

//------------------------------------------------------------------------------

static uint8_t s_pin_cs;
static uint8_t s_pin_wr;
static uint8_t s_pin_data;

//------------------------------------------------------------------------------

static inline void ht1621_send_bit(uint32_t data, uint8_t bit) {
  gpio_put(s_pin_wr, 0);
  gpio_put(s_pin_data, (data >> bit) & 1);
  sleep_us(HT1621_DELAY_US);
  gpio_put(s_pin_wr, 1);
  sleep_us(HT1621_DELAY_US);
}

static void ht1621_send_bits(uint32_t data, uint8_t bit_count) {
  for (int i = bit_count - 1; i >= 0; i--)
    ht1621_send_bit(data, i);
}

static inline void ht1621_send_byte(uint8_t data) {
  ht1621_send_bits(data, 8);
}

static inline void ht1621_send_data(uint8_t data) {
  for (int i = 0; i < 4; i++)
    ht1621_send_bit(data, i);
}

static inline void ht1621_send_addr(uint8_t addr) {
  ht1621_send_bits(addr, 6);
}

static inline void ht1621_send_wr(uint8_t addr) {
  ht1621_send_bits(HT1621_ID_WRITE, 3);
  ht1621_send_addr(addr);
}

static inline void ht1621_select(void) {
  gpio_put(s_pin_cs, 0);
  sleep_us(HT1621_DELAY_US);
}

static inline void ht1621_deselect(void) {
  gpio_put(s_pin_cs, 1);
  sleep_us(HT1621_DELAY_US);
}

static void ht1621_send_cmd(uint8_t cmd) {
  ht1621_select();
  ht1621_send_bits(HT1621_ID_CMD, 3);
  ht1621_send_byte(cmd);     // command code
  ht1621_send_bits(0x00, 1); // stop bit
  ht1621_deselect();
}

static void ht1621_gpio_init(uint8_t pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, 1);
}

//------------------------------------------------------------------------------

void ht1621_init(uint8_t pin_cs, uint8_t pin_wr, uint8_t pin_data) {
  s_pin_cs = pin_cs;
  ht1621_gpio_init(pin_cs);

  s_pin_wr = pin_wr;
  ht1621_gpio_init(pin_wr);

  s_pin_data = pin_data;
  ht1621_gpio_init(pin_data);

  sleep_ms(50); // Power-up stabilization delay

  // Initialize HT1621 internal oscillator and LCD bias
  ht1621_send_cmd(HT1621_CMD_SYS_EN);
  ht1621_send_cmd(HT1621_CMD_BIAS);
  ht1621_send_cmd(HT1621_CMD_LCD_ON);

  ht1621_clear_block(0, HT1621_RAM_SIZE);
}

void ht1621_write(uint8_t addr, uint8_t data) {
  ht1621_select();
  ht1621_send_wr(addr);
  ht1621_send_data(data);
  ht1621_deselect();
}

void ht1621_write_block(uint8_t addr, const uint8_t *data, uint8_t length) {
  ht1621_select();
  ht1621_send_wr(addr);

  for (uint8_t i = 0; i < length; i++)
    ht1621_send_data(data[i]);

  ht1621_deselect();
}

void ht1621_clear_block(uint8_t addr, uint8_t length) {
  ht1621_select();
  ht1621_send_wr(addr);

  for (uint8_t i = 0; i < length; i++)
    ht1621_send_data(0);

  ht1621_deselect();
}
