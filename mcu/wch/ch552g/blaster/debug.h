/* Provide printf subroutine and delay function */

#pragma once

#include <stdint.h>
#include "ch554.h"

#ifndef UART0_BAUD
#define UART0_BAUD 9600
#endif

#ifndef UART1_BAUD
#define UART1_BAUD 9600
#endif

void delay_us(uint16_t n); // Delay in units of uS
void delay_ms(uint16_t n); // Delay in mS

/*******************************************************************************
* Function Name  : CfgFsys()
* Description    : CH554 clock selection and configuration. Defaults to 6MHz.
*                  Fsys = Fosc * 4 / (CLOCK_CFG & MASK_SYS_CK_SEL)
*******************************************************************************/
inline void CfgFsys(void) {
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
//  CLOCK_CFG |= bOSC_EN_XT;        // Enable external crystal
//  CLOCK_CFG &= ~bOSC_EN_INT;      // Disable internal crystal

#if   FREQ_SYS == 32000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x07;  // 32MHz
#elif FREQ_SYS == 24000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x06;  // 24MHz
#elif FREQ_SYS == 16000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x05;  // 16MHz
#elif FREQ_SYS == 12000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x04;  // 12MHz
#elif FREQ_SYS == 6000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x03;  // 6MHz
#elif FREQ_SYS == 3000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x02;  // 3MHz
#elif FREQ_SYS == 750000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x01;  // 750KHz
#elif FREQ_SYS == 187500
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x00;  // 187.5KHz
#else
    #warning FREQ_SYS invalid or not set
#endif

    SAFE_MOD = 0x00;
}

/*******************************************************************************
* Function Name  : UART0Alter()
* Description    : CH554 serial port 0 pin mapping
*******************************************************************************/
inline void UART0Alter(void) {
    PIN_FUNC |= bUART0_PIN_X; // Serial port mapped to P1.2 and P1.3
}

/*******************************************************************************
* Function Name  : InitSTDIO()
* Description    : CH554 serial port 0 is initialized, T1 is used as the baud
*                  rate generator of UART0 by default, T2 can also be used as
*                  a baud rate generator
*******************************************************************************/
inline void InitSTDIO(void) {
    uint32_t x;
    uint8_t x2;

    SM0 = 0;
    SM1 = 1;
    SM2 = 0;      // Serial port 0 usage mode 1

    // Use Timer1 as a baud rate generator
    RCLK = 0;     // UART0 receive clock
    TCLK = 0;     // UART0 transmit clock
    PCON |= SMOD;

    // If you change the main frequency, be careful not to overflow the value of x
    x = 10 * FREQ_SYS / UART0_BAUD / 16;
    x2 = x % 10;
    x /= 10;
    if (x2 >= 5)
      x++; // Rounding

    // 0X20, Timer1 as 8-bit auto-reload timer
    TMOD = TMOD & ~(bT1_GATE | bT1_CT | MASK_T1_MOD) | bT1_M1;
    T2MOD = T2MOD | bTMR_CLK | bT1_CLK; // Timer1 clock selection
    TH1 = -x; // 12MHz crystal oscillator, buad / 12 is the actual need to set the baud rate
    TR1 = 1;  // Start timer 1
    TI = 1;
    REN = 1;  // Serial 0 receive enable
}

/*******************************************************************************
* Function Name  : UART0RcvByte()
* Description    : CH554UART0 receives a byte
* Return         : SBUF
*******************************************************************************/
inline uint8_t UART0RcvByte(void) {
    while (RI == 0); // Wait for uart rx interrupt flag
    RI = 0;
    return SBUF;
}

/*******************************************************************************
* Function Name  : UART0SendByte(uint8_t SendDat)
* Description    : CH554UART0 sends a byte
* Input          : uint8_t SendDat; the data to be sent
*******************************************************************************/
inline void UART0SendByte(uint8_t SendDat) {
    SBUF = SendDat;
    while (TI == 0); // Wait for transmit to finish (TI == 1)
    TI = 0;
}

/*******************************************************************************
* Function Name  : CH554UART1Alter()
* Description    : Set the alternate pin mappings for UART1:
*                  TX on P3.2, RX on P3.4
*******************************************************************************/
inline void UART1Alter(void) {
    PIN_FUNC |= bUART1_PIN_X;
}

/*******************************************************************************
* Function Name  : UART1Setup()
* Description    : CH554 UART1 initialization
*******************************************************************************/
inline void UART1Setup(void) {
    U1SM0 = 0;  // UART1 select 8-bit data width
    U1SMOD = 1; // Fast mode
    U1REN = 1;  // Enable receive

    // Should correct for rounding in SBAUD1 calculation
    SBAUD1 = (uint8_t)(256 - FREQ_SYS / 16 / UART1_BAUD);
}

/*******************************************************************************
* Function Name  : UART1RcvByte()
* Description    : CH554 UART1 receive one byte
* Return         : SBUF
*******************************************************************************/
inline uint8_t UART1RcvByte(void) {
    while (U1RI == 0);
    U1RI = 0;
    return SBUF1;
}

/*******************************************************************************
* Function Name  : UART1SendByte(uint8_t SendDat)
* Description    : CH554 UART1 send one byte
* Input          : uint8_t SendDat  data to be sent
*******************************************************************************/
inline void UART1SendByte(uint8_t SendDat) {
    while (U1TI == 0);
    U1TI = 0;
    SBUF1 = SendDat;
}

/*******************************************************************************
* Function Name  : WDTModeSelect(uint8_t mode)
* Description    : CH554 watchdog mode selection
* Input          : uint8_t mode
                   0  timer
                   1  watchDog
* Output         : None
* Return         : None
*******************************************************************************/
inline void WDTModeSelect(uint8_t mode) {
   SAFE_MOD = 0x55;
   SAFE_MOD = 0xAA; // Enter Safe Mode
   if (mode)
     GLOBAL_CFG |= bWDOG_EN;  // Start watchdog reset
   else
     GLOBAL_CFG &= ~bWDOG_EN; // Start watchdog only as a timer
   SAFE_MOD = 0x00; // Exit safe Mode
   WDOG_COUNT = 0;  // Watchdog assignment initial value
}

/*******************************************************************************
* Function Name  : WDTFeed(uint8_t tim)
* Description    : CH554 watchdog timer time setting
* Input          : uint8_t tim watchdog reset time setting

                   00H(6MHz)=2.8s
                   80H(6MHz)=1.4s
* Output         : None
* Return         : None
*******************************************************************************/
inline void WDTFeed(uint8_t tim) {
   WDOG_COUNT = tim; // Watchdog counter assignment
}

inline int putchar(int c) {
    UART1SendByte(c);
    return c;
}
