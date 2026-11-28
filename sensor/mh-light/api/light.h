#ifndef LIGHT_H
#define LIGHT_H

#include <stdbool.h>
#include <stdint.h>

//------------------------------------------------------------------------------

// Configure the pin as input and load the initial state.
void light_init(uint8_t pin_data);

// Call periodically (e.g. every 10 ms). Returns true when the filtered
// state has just changed.
bool light_update(void);

// Filtered state, valid after light_init()/light_update().
bool light_is_bright(void);

#endif // LIGHT_H
