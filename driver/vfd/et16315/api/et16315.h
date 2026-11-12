#ifndef ET16315_H
#define ET16315_H

#include <stdbool.h>
#include <stdint.h>

// Display RAM: 12 digits x 3 bytes (24 segments), addresses 0x00..0x23
#define ET16315_RAM_SIZE 36

typedef enum {
  ET16315_DUTY_1_16,   // default
  ET16315_DUTY_2_16,   // 1/8
  ET16315_DUTY_4_16,   // 1/4
  ET16315_DUTY_10_16,  // 5/8
  ET16315_DUTY_11_16,
  ET16315_DUTY_12_16,  // 3/4
  ET16315_DUTY_13_16,
  ET16315_DUTY_14_16   // 7/8
} et16315_duty_t;

typedef enum {
  ET16315_GRID_4,   // 24 segments
  ET16315_GRID_5,   // 23 segments
  ET16315_GRID_6,   // 22 segments
  ET16315_GRID_7,   // 21 segments
  ET16315_GRID_8,   // 20 segments
  ET16315_GRID_9,   // 19 segments
  ET16315_GRID_10,  // 18 segments
  ET16315_GRID_11,  // 17 segments
  ET16315_GRID_12   // 16 segments
} et16315_mode_t;

// Initialize ET16315 hardware pins and internal states
// (12 digits / 16 segments, display ON, max brightness, all LEDs off)
// NOTE: Requires power-up stabilization delay 200ms
void et16315_init(uint8_t pin_stb, uint8_t pin_clk, uint8_t pin_dout, uint8_t pin_din);

// Select number of digits (4..12), the rest of the 24 segments is
// available as SG17..SG24 (12 digits = 16 segments ... 4 digits = 24 segments).
// NOTE: this turns the display OFF, call et16315_display() afterwards
void et16315_set_mode(et16315_mode_t mode);

// Display on/off and brightness (0..7 = 1/16, 2/16, 4/16, 10/16 .. 14/16)
void et16315_display(bool on, et16315_duty_t duty);

// Clear a block of RAM
void et16315_clear_block(uint8_t addr, uint8_t length);

// Write a block of RAM
void et16315_write_block(uint8_t addr, const uint8_t *data, uint8_t length);

// Write a single byte to a specific RAM address
void et16315_write(uint8_t addr, uint8_t data);

// Set LED1..LED4 (bit0..bit3, 0 = LED lit)
void et16315_set_leds(uint8_t leds);

// Read key matrix (16 x 2), 4 bytes
void et16315_read_keys(uint8_t keys[4]);

// Test a key in data returned by et16315_read_keys(), ks = 0..15, k = 0..1
bool et16315_key_pressed(const uint8_t keys[4], uint8_t ks, uint8_t k);

#endif // ET16315_H
