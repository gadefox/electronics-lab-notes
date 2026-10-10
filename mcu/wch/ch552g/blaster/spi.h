/********************************** (C) COPYRIGHT *******************************
* Author             : WCH
* Version            : V1.0
* Date               : 2017/07/05
* Description        : CH554 SPI master/slave mode interface functions
* Note: When chip select is asserted, the slave will automatically load the preset
* value from SPI0_S_PRE into the transmit shift buffer. It is best to write the
* pre-send value to the SPI0_S_PRE register before chip select is asserted, or
* have the master discard the first received byte. Note that during transmission
* the master will first fetch the value in SPI0_S_PRE, generating one S0_IF_BYTE
* interrupt.
* If chip select transitions from inactive to active and the slave is transmitting
* first, it is best to place the first output byte in SPI0_S_PRE; if chip select
* is already active, use SPI0_DATA for data.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
********************************************************************************/

#pragma once

#include <stdint.h>
#include "ch554.h"

#define SPI_CK_SET(n) (SPI0_CK_SE = n)

#define SPIMasterAssertCS()   (SCS = 0)
#define SPIMasterDeassertCS() (SCS = 1)

/*******************************************************************************
* Function Name  : SPIMasterModeSetLSB
* Description    : SPI master mode initialization
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
inline void SPIMasterModeSetLSB(void) {
    SPI0_SETUP = 0x08;
    SPI0_CTRL  = 0x00;
}

/*******************************************************************************
* Function Name  : SPIMasterModeSetIdleLow()
* Description    : SPI master mode init: SCK idle-low, output disabled
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
inline void SPIMasterModeSetIdleLow(void) {
    SPI0_CTRL = 0x00;
}

/*******************************************************************************
* Function Name  : SPIMasterModeSetIdleLowOut()
* Description    : SPI master mode init: SCK idle-low, output enabled
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
inline void SPIMasterModeSetIdleLowOut(void) {
    SPI0_CTRL = 0x60;
}

/*******************************************************************************
* Function Name  : SPIInterruptInit()
* Description    : Interrupt initialization
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
inline void SPIInterruptInit(void) {
    SPI0_SETUP |= bS0_IE_FIFO_OV | bS0_IE_BYTE;   // Enable receive-1-byte interrupt and FIFO overflow interrupt
    SPI0_CTRL  |= bS0_AUTO_IF;                    // Auto-clear S0_IF_BYTE interrupt flag
    SPI0_STAT  |= 0xFF;                           // Clear all SPI0 interrupt flags

#ifdef SPI_Interrupt
    IE_SPI0 = 1;                                   // Enable SPI0 interrupt
#endif
}

/*******************************************************************************
* Function Name  : SPIMasterWrite(UINT8 dat)
* Description    : SPI write, master mode
* Input          : UINT8 dat   data byte
* Output         : None
* Return         : None
*******************************************************************************/
inline void SPIMasterWrite(uint8_t dat) {
    SPI0_DATA = dat;
    while (S0_FREE == 0);   // Wait for transfer to complete
    // If bS0_DATA_DIR is 1, one byte can be read directly here for fast read/write
}

/*******************************************************************************
* Function Name  : SPIMasterRead( )
* Description    : SPI0 read, master mode
* Input          : None
* Output         : None
* Return         : UINT8 ret
*******************************************************************************/
inline uint8_t SPIMasterRead(void) {
    SPI0_DATA = 0xFF;
    while (S0_FREE == 0);
    return SPI0_DATA;
}

/*******************************************************************************
* Function Name  : SPISlvModeSet( )
* Description    : SPI slave mode initialization
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
inline void SPISlvModeSet(void) {
    SPI0_SETUP = 0x80;   // Slave mode, MSB-first
    SPI0_CTRL  = 0x89;   // Read/write FIFO, auto-clear S0_IF_BYTE flag
    P1_MOD_OC &= 0x0F;
    P1_DIR_PU &= 0x0F;   // SCS, MOSI, SCK, MISO all set to floating input
}

/*******************************************************************************
* Function Name  : SPISlvWrite(UINT8 dat)
* Description    : SPI write, slave mode
* Input          : UINT8 dat   data byte
* Output         : None
* Return         : None
*******************************************************************************/
inline void SPISlvWrite(uint8_t dat) {
    SPI0_DATA = dat;
    while (S0_FREE == 0);
}

/*******************************************************************************
* Function Name  : SPISlvRead( )
* Description    : SPI0 read, slave mode
* Input          : None
* Output         : None
* Return         : UINT8 ret
*******************************************************************************/
inline uint8_t SPISlvRead(void) {
    while (S0_FREE == 0);
    return SPI0_DATA;
}
