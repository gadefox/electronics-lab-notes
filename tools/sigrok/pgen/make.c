#include <stdio.h>
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#define GPIO_D_MASK 0x7FFFF8

int main() {
  set_sys_clock_khz(96000, true);
  stdio_init_all();

  gpio_set_function(2, GPIO_FUNC_PWM);
  gpio_set_slew_rate(2, GPIO_SLEW_RATE_FAST);
  gpio_set_drive_strength(2, GPIO_DRIVE_STRENGTH_8MA);

  uint slice = pwm_gpio_to_slice_num(2);
  pwm_set_clkdiv_int_frac(slice, 1, 0);      // div 1.0
  pwm_set_wrap(slice, 11);                   // period = 12 cycles -> 96/12 = 8 MHz
  pwm_set_chan_level(slice, PWM_CHAN_A, 6);  // 50 % duty
  pwm_set_enabled(slice, true);

  gpio_init_mask(GPIO_D_MASK);
  gpio_set_dir_masked(GPIO_D_MASK, GPIO_D_MASK);

  uint32_t tstart = time_us_32();
  while (true) {
    uint32_t cval = (time_us_32() - tstart) >> 5;
    gpio_put_masked(GPIO_D_MASK, cval << 3);
  }
}
