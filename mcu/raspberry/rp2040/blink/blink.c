#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 25

int main(void)
{
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  while (1) {
    printf("LED is on\n");
    gpio_put(LED_PIN, 1);
    sleep_ms(300);

    printf("LED is off\n");
    gpio_put(LED_PIN, 0);
    sleep_ms(300);
  }
}
