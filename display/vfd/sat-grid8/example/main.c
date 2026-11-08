#include <stdio.h>
#include "pico/status_led.h"
#include "pico/stdlib.h"
#include "button.h"
#include "display.h"
#include "pt6315.h"

static int s_state;

#define NUM_GRIDS 9
#define NUM_SEGS  19

static uint8_t cur_grid = 0;
static uint8_t cur_seg  = 0;

static void on_button(void) {
  status_led_set_state(!(s_state % 2));

  uint8_t buf[3] = {0, 0, 0};

  // SG1..SG8 -> byte 0, SG9..SG16 -> byte 1, SG17..SG24 -> byte 2
  buf[cur_seg >> 3] = (uint8_t)(1u << (cur_seg & 7));

  pt6315_clear_block(0, PT6315_RAM_SIZE);
  pt6315_write_block(cur_grid * 3, buf, 3);

  printf("grid=%u seg=%u (addr=0x%02X byte=%u bit=%u)\n",
         cur_grid, cur_seg, cur_grid * 3, cur_seg >> 3, cur_seg & 7);

  if (++cur_seg >= NUM_SEGS) {
    cur_seg = 0;
    if (++cur_grid >= NUM_GRIDS)
      cur_grid = 0;
  }

  s_state++;
}

int main() {
  stdio_init_all();

  // Initialize hardware modules
  status_led_init();
  button_init(PICO_DEFAULT_BUTTON_PIN);
  button_set_callback(on_button);

  while (!stdio_usb_connected())
    sleep_ms(10);
  sleep_ms(200);

  pt6315_init(16, 21, 29, 28); // STB, CLK, DIN, DOUT
  pt6315_set_mode(PT6315_GRID_9);
  pt6315_clear_block(0, PT6315_RAM_SIZE);
  pt6315_display(true, PT6315_DUTY_14_16);
  pt6315_set_leds(0x01);

  while (true) {
    // Background tasks (non-blocking)
    button_task_update();

    // Small sleep or let it run free-spin with tight loop
    tight_loop_contents();
  }
}
