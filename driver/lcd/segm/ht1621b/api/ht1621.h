#ifndef HT1621_H
#define HT1621_H

#include <stdint.h>

// Initialize HT1621 hardware pins and internal states
void ht1621_init(uint8_t pin_cs, uint8_t pin_wr, uint8_t pin_data);

// Clear a block of RAM
void ht1621_clear_block(uint8_t addr, uint8_t length);

// Write a block of RAM
void ht1621_write_block(uint8_t addr, const uint8_t *data, uint8_t length);

// Write a single nibble/byte to a specific RAM address
void ht1621_write(uint8_t addr, uint8_t data);

#endif // HT1621_H
