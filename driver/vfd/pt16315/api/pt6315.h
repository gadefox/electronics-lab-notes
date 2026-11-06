#ifndef PT6315_H
#define PT6315_H

#include <stdbool.h>
#include <stdint.h>

// Display RAM: 12 digits x 3 bytes (24 segments), addresses 0x00..0x23
#define PT6315_RAM_SIZE 36

typedef enum {
    PT6315_DUTY_1_16  = 0,  // default
    PT6315_DUTY_2_16  = 1,  // 1/8
    PT6315_DUTY_4_16  = 2,  // 1/4
    PT6315_DUTY_10_16 = 3,  // 5/8
    PT6315_DUTY_11_16 = 4,
    PT6315_DUTY_12_16 = 5,  // 3/4
    PT6315_DUTY_13_16 = 6,
    PT6315_DUTY_14_16 = 7   // 7/8
} pt6315_duty_t;

// Initialize PT6315 hardware pins and internal states
// (12 digits / 16 segments, display ON, max brightness, all LEDs off)
// NOTE: Requires power-up stabilization delay 200ms
void pt6315_init(uint8_t pin_stb, uint8_t pin_clk, uint8_t pin_din,
                 uint8_t pin_dout);

// Select number of digits (4..12), the rest of the 24 segments is
// available as SG17..SG24 (12 digits = 16 segments ... 4 digits = 24 segments).
// NOTE: this turns the display OFF, call pt6315_display() afterwards
void pt6315_set_mode(uint8_t digits);

// Display on/off and brightness (0..7 = 1/16, 2/16, 4/16, 10/16 .. 14/16)
void pt6315_display(bool on, pt6315_duty_t duty);

// Clear a block of RAM
void pt6315_clear_block(uint8_t addr, uint8_t length);

// Write a block of RAM
void pt6315_write_block(uint8_t addr, const uint8_t *data, uint8_t length);

// Write a single byte to a specific RAM address
void pt6315_write(uint8_t addr, uint8_t data);

// Set LED1..LED4 (bit0..bit3, 0 = LED lit)
void pt6315_set_leds(uint8_t leds);

// Read key matrix (16 x 2), 4 bytes
void pt6315_read_keys(uint8_t keys[4]);

// Test a key in data returned by pt6315_read_keys(), ks = 0..15, k = 0..1
bool pt6315_key_pressed(const uint8_t keys[4], uint8_t ks, uint8_t k);

#endif // PT6315_H
