#include "pico/stdlib.h"

#define PIN_COM 0
#define PIN_SEG 1

int main(void)
{
  gpio_init(PIN_COM);
  gpio_set_dir(PIN_COM, GPIO_OUT);
  gpio_init(PIN_SEG);
  gpio_set_dir(PIN_SEG, GPIO_OUT);

  while (true) {
    gpio_put(PIN_COM, 0);
    gpio_put(PIN_SEG, 1);
    sleep_ms(8);

    gpio_put(PIN_COM, 1);
    gpio_put(PIN_SEG, 0);
    sleep_ms(8);
  }
}
