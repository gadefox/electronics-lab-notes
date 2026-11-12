#include "pico/stdlib.h"
#include "button.h"

//------------------------------------------------------------------------------

#define DEBOUNCE_DELAY_US 30000u // Debounce threshold in milliseconds

//------------------------------------------------------------------------------

static uint8_t s_button_pin;
static bool s_last_raw_state = true;     // Assuming active-low with pull-up (true = released)
static bool s_stable_state = true;       // Debounced stable state
static uint32_t s_last_debounce_time = 0;
static button_callback_t s_click_callback = NULL;

//------------------------------------------------------------------------------

void button_init(uint8_t pin) {
  s_button_pin = pin;

  gpio_init(s_button_pin);
  gpio_set_dir(s_button_pin, GPIO_IN);
  gpio_pull_up(s_button_pin); // Enable internal pull-up (button connects pin to GND when pressed)

  s_last_raw_state = gpio_get(s_button_pin);
  s_stable_state = s_last_raw_state;
}

void button_set_callback(button_callback_t cb) {
  s_click_callback = cb;
}

void button_task_update(void) {
  uint32_t now = time_us_32();
  bool raw_state = gpio_get(s_button_pin);

  // If the switch changed, reset the debounce timer
  if (raw_state != s_last_raw_state) {
    s_last_raw_state = raw_state;
    s_last_debounce_time = now;
  }

  // If the reading has been stable for longer than the debounce delay
  if (raw_state != s_stable_state && now - s_last_debounce_time >= DEBOUNCE_DELAY_US) {
    s_stable_state = raw_state;

    // Detect falling edge (button pressed: HIGH -> LOW for active-low)
    if (!s_stable_state && s_click_callback)
      s_click_callback(); // Trigger the registered event
  }
}
