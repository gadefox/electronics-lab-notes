#include "pico/stdlib.h"
#include "vk1s56e.h"

//------------------------------------------------------------------------------

// VK1S56E command codes
#define VK1S56E_CMD_SYS_EN  0x01 // system enable
#define VK1S56E_CMD_LCD_ON  0x03 // LCD on
#define VK1S56E_CMD_BIAS    0x29 // 1/3 bias, 1/4 duty (COM0-3)

#define VK1S56E_ID_CMD    0x04 // 0b100
#define VK1S56E_ID_WRITE  0x05 // 0b101

// WR half-period in microseconds.
// Datasheet (VK1S56E): min WR pulse width in write mode is 3.34 us @
// VDD=3V (1.67 us @ 5V), max WR clock is 150 kHz @ 3V. 4 us per phase -> 8 us
// period -> 125 kHz, which satisfies the 3V limits and therefore the 5V ones.
#define VK1S56E_DELAY_US  4

//------------------------------------------------------------------------------

static uint8_t s_pin_cs;
static uint8_t s_pin_wr;
static uint8_t s_pin_data;

//------------------------------------------------------------------------------

static inline void vk1s56e_send_bit(uint32_t data, uint8_t bit) {
  gpio_put(s_pin_wr, 0);
  gpio_put(s_pin_data, (data >> bit) & 1);
  sleep_us(VK1S56E_DELAY_US);
  gpio_put(s_pin_wr, 1);
  sleep_us(VK1S56E_DELAY_US);
}

static void vk1s56e_send_bits(uint32_t data, uint8_t bit_count) {
  for (int i = bit_count - 1; i >= 0; i--)
    vk1s56e_send_bit(data, i);
}

static inline void vk1s56e_send_byte(uint8_t data) {
  vk1s56e_send_bits(data, 8);
}

static inline void vk1s56e_send_data(uint8_t data) {
  for (int i = 0; i < 4; i++)
    vk1s56e_send_bit(data, i);
}

static inline void vk1s56e_send_addr(uint8_t addr) {
  vk1s56e_send_bits(addr, 6);
}

static inline void vk1s56e_send_wr(uint8_t addr) {
  vk1s56e_send_bits(VK1S56E_ID_WRITE, 3);
  vk1s56e_send_addr(addr);
}

static inline void vk1s56e_select(void) {
  gpio_put(s_pin_cs, 0);
  sleep_us(VK1S56E_DELAY_US);
}

static inline void vk1s56e_deselect(void) {
  gpio_put(s_pin_cs, 1);
  sleep_us(VK1S56E_DELAY_US);
}

static void vk1s56e_send_cmd(uint8_t cmd) {
  vk1s56e_select();
  vk1s56e_send_bits(VK1S56E_ID_CMD, 3);
  vk1s56e_send_byte(cmd);     // command code
  vk1s56e_send_bits(0x00, 1); // stop bit
  vk1s56e_deselect();
}

static void vk1s56e_gpio_init(uint8_t pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, 1);
}

//------------------------------------------------------------------------------

void vk1s56e_init(uint8_t pin_cs, uint8_t pin_wr, uint8_t pin_data) {
  s_pin_cs = pin_cs;
  vk1s56e_gpio_init(pin_cs);

  s_pin_wr = pin_wr;
  vk1s56e_gpio_init(pin_wr);

  s_pin_data = pin_data;
  vk1s56e_gpio_init(pin_data);

  sleep_ms(50); // Power-up stabilization delay

  // Initialize VK1S56D internal oscillator and LCD bias
  vk1s56e_send_cmd(VK1S56E_CMD_SYS_EN);
  vk1s56e_send_cmd(VK1S56E_CMD_BIAS);
  vk1s56e_send_cmd(VK1S56E_CMD_LCD_ON);

  vk1s56e_clear_block(0, VK1S56E_RAM_SIZE);
}

void vk1s56e_write(uint8_t addr, uint8_t data) {
  vk1s56e_select();
  vk1s56e_send_wr(addr);
  vk1s56e_send_data(data);
  vk1s56e_deselect();
}

void vk1s56e_write_block(uint8_t addr, const uint8_t *data, uint8_t length) {
  vk1s56e_select();
  vk1s56e_send_wr(addr);

  for (uint8_t i = 0; i < length; i++)
    vk1s56e_send_data(data[i]);

  vk1s56e_deselect();
}

void vk1s56e_clear_block(uint8_t addr, uint8_t length) {
  vk1s56e_select();
  vk1s56e_send_wr(addr);

  for (uint8_t i = 0; i < length; i++)
    vk1s56e_send_data(0);

  vk1s56e_deselect();
}
