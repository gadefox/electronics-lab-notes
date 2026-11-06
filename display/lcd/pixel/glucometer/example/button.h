#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

// Callback function pointer type for click events
typedef void (*button_callback_t)(void);

// Initialize button pin and internal state
void button_init(uint8_t pin);

// Register a callback function for button press event
void button_set_callback(button_callback_t cb);

// Periodic background task to handle reading and debouncing (call in main loop)
void button_task_update(void);

#endif // BUTTON_H
