/********************************** (C) COPYRIGHT *******************************
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        : CH554 DEBUG Interface
*                      CH554 main frequency modification, delay function definition
*                      Serial port 0 and serial port 1 initialization
*                      Serial port 0 and serial port 1 transceiver subfunctions
*                      Watchdog initialization
*******************************************************************************/

#include <stdint.h>

#include "ch554.h"
#include "debug.h"

/*******************************************************************************
* Function Name  : mDelayuS(uint16_t n)
* Description    : Delay in microseconds
* Input          : uint16_t n
*******************************************************************************/
void delay_us(uint16_t n) {
#if   FREQ_SYS <= 750000
    n >>= 8;
#elif FREQ_SYS <= 3000000
    n >>= 4;
#elif FREQ_SYS <= 6000000
    n >>= 2;
#endif

    while (n) {  // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
        SAFE_MOD++;  // 2 Fsys cycles; for higher Fsys, add operations here
#if FREQ_SYS >= 14000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 16000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 18000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 20000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 22000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 24000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 26000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 28000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 30000000
        SAFE_MOD++;
#endif
#if FREQ_SYS >= 32000000
        SAFE_MOD++;
#endif
        n--;
    }
}

/*******************************************************************************
* Function Name  : mDelaymS(uint16_t n)
* Description    : Delay in milliseconds
* Input          : uint16_t n
*******************************************************************************/
void delay_ms(uint16_t n) {
    while (n) {
        delay_us(1000);
        n--;
    }
}
