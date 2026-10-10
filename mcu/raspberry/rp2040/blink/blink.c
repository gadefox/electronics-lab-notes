#include "hardware/gpio.h"
#include "pico/time.h"

#define LED_PIN 25

int main(void)
{
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  while (1) {
    gpio_put(LED_PIN, 1);
    sleep_ms(300);

    gpio_put(LED_PIN, 0);
    sleep_ms(300);
  }
}
