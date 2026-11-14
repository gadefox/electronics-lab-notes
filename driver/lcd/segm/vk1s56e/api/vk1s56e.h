#ifndef VK1S56E_H
#define VK1S56E_H

#include <stdint.h>

// VK1S56E RAM
#define VK1S56E_RAM_SIZE  27

// Initialize VK1S56 hardware pins and internal states
void vk1s56e_init(uint8_t pin_cs, uint8_t pin_wr, uint8_t pin_data);

// Clear a block of RAM
void vk1s56e_clear_block(uint8_t addr, uint8_t length);

// Write a block of RAM
void vk1s56e_write_block(uint8_t addr, const uint8_t *data, uint8_t length);

// Write a single nibble/byte to a specific RAM address
void vk1s56e_write(uint8_t addr, uint8_t data);

#endif // VK1S56E_H
