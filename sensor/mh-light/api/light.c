#include "gpio.h"
#include "light.h"

//------------------------------------------------------------------------------

#ifndef LIGHT_DEBOUNCE
#define LIGHT_DEBOUNCE  5         // consecutive samples to accept a change
#endif

//------------------------------------------------------------------------------

static uint8_t s_pin_data;
static bool s_state;
static uint8_t s_counter;

//------------------------------------------------------------------------------

static inline bool light_get_state(void) {
  return (gpio_read(s_pin_data) != 0);
}

static inline void light_update_state(bool state) {
  s_state = state;
  s_counter = 0;
}

//------------------------------------------------------------------------------

void light_init(uint8_t pin_data) {
  s_pin_data = pin_data;
  gpio_init_input(pin_data);

  bool state = light_get_state();
  light_update_state(state);
}

bool light_update(void) {
  bool state = light_get_state();

  if (state == s_state) {
    s_counter = 0; // no pending change
    return false;
  }

  if (++s_counter < LIGHT_DEBOUNCE)
    return false;

  light_update_state(state);
  return true;
}

bool light_is_bright(void) {
  return s_state;
}
