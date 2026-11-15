#ifndef VFD_SAT_RECEIVER_H
#define VFD_SAT_RECEIVER_H

#include <stdint.h>

#define VFD_ICON_COLON1   (1 << 0)
#define VFD_ICON_COLON2   (1 << 1)
#define VFD_ICON_COLON3   (1 << 2)
#define VFD_ICON_SAT      (1 << 3)
#define VFD_ICON_TER      (1 << 4)
#define VFD_ICON_FAV      (1 << 5)
#define VFD_ICON_TV       (1 << 6)
#define VFD_ICON_RADIO    (1 << 7)
#define VFD_ICON_MPEG4    (1 << 8)
#define VFD_ICON_MPEG2    (1 << 9)
#define VFD_ICON_HD       (1 << 10)
#define VFD_ICON_EUR      (1 << 11)
#define VFD_ICON_PLAY     (1 << 12)
#define VFD_ICON_BACK     (1 << 13)
#define VFD_ICON_PAUSE    (1 << 14)
#define VFD_ICON_FORWARD  (1 << 15)
#define VFD_ICON_STOP     (1 << 16)
#define VFD_ICON_ARROW    (1 << 17)
#define VFD_ICON_ALARM    (1 << 18)
#define VFD_ICON_TIME     (1 << 19)
#define VFD_ICON_BOX      (1 << 20)
#define VFD_ICON_ONE      (1 << 21)
#define VFD_ICON_TWO      (1 << 22)
#define VFD_ICON_KEY      (1 << 23)
#define VFD_ICON_16_9     (1 << 24)
#define VFD_ICON_USB      (1 << 25)
#define VFD_ICON_DV3      (1 << 26)
#define VFD_ICON_MAX  27

void vfd_init(uint8_t pin_stb, uint8_t pin_clk, uint8_t pin_dout, uint8_t pin_din);
void vfd_clear(void);
void vfd_draw_text(const char *text, uint8_t grid_pos);
void vfd_draw_num(int16_t value, uint8_t decimal_pos);
void vfd_set_icons(uint32_t icon_mask);
void vfd_flush(void);

#endif // VFD_SAT_RECEIVER_H
