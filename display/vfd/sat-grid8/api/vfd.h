#ifndef VFD_SAT_RECEIVER_H
#define VFD_SAT_RECEIVER_H

#include <stdint.h>

#define VFD_SEG_COLON  15
#define VFD_SEG_ICON   16

#define VFD_GRID8_EUR      0
#define VFD_GRID8_PLAY     1
#define VFD_GRID8_BACK     2
#define VFD_GRID8_PAUSE    3
#define VFD_GRID8_FORWARD  4
#define VFD_GRID8_STOP     6
#define VFD_GRID8_ARROW    7
#define VFD_GRID8_ALARM    8
#define VFD_GRID8_TIME     9
#define VFD_GRID8_BOX      10
#define VFD_GRID8_ONE      11
#define VFD_GRID8_TWO      12
#define VFD_GRID8_KEY      13
#define VFD_GRID8_16_9     14
#define VFD_GRID8_USB      15
#define VFD_GRID8_DV3      16

void vfd_init(uint8_t pin_stb, uint8_t pin_clk, uint8_t pin_dout, uint8_t pin_din);
void vfd_draw_text(const char *text, uint8_t grid_pos);
void vfd_draw_num(int16_t value, uint8_t decimal_pos);
void vfd_flush(void);

#endif // VFD_SAT_RECEIVER_H
