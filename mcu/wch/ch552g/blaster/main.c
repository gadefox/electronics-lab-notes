// USB-Blaster inslen CH55x MCU.
// Author: Duan
// License: MIT
// Based on USB-MIDI by Zhiyuan Wan

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ch554.h"
#include "ch554_usb.h"
#include "debug.h"
#include "ftdi.h"
#include "spi.h"

// gpio
#define TDI MOSI // P1.5
#define TDO MISO // P1.6
#define TCK SCK  // P1.7
SBIT(TMS, 0xB0, 1); // P3.1
SBIT(LED, 0xB0, 2); // P3.2

// bit-bang
SBIT(P2B7, 0xA0, 7);
SBIT(P2B6, 0xA0, 6);
SBIT(P2B5, 0xA0, 5);
SBIT(P2B4, 0xA0, 4);
SBIT(P2B3, 0xA0, 3);
SBIT(P2B2, 0xA0, 2);
SBIT(P2B1, 0xA0, 1);
SBIT(P2B0, 0xA0, 0);

__xdata __at(0x0000) uint8_t transmit_buffer[128];
__xdata __at(0x0080) uint8_t receive_buffer[64];
__xdata __at(0x00C0) uint8_t SerialNum[22];
/* 0x00D6 */
__xdata __at(0x0100) uint8_t Ep0Buffer[DEFAULT_ENDP0_SIZE]; // Endpoint 0 OUT & IN buffer, must be an even address
__xdata __at(0x0140) uint8_t Ep1Buffer[64]; // Endpoint 1 IN buffer
__xdata __at(0x0180) uint8_t Ep2Buffer[64]; // Endpoint 2 OUT buffer, must be an even address
/* 0x01C0 */

const uint8_t *Descr;
uint16_t SetupLen;
uint8_t SetupReq, UsbConfig, PendingAddr;

#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

/* 128 bytes */
__code uint8_t ftdi_rom[] = {
    0x00, 0x00, 0xFB, 0x09, 0x01, 0x60, 0x00, 0x04, 0x80, 0xE1, 0x1C, 0x00,
    0x00, 0x02, 0x94, 0x0E, 0xA2, 0x18, 0xBA, 0x12, 0x0E, 0x03, 0x41, 0x00,
    0x6C, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x61, 0x00, 0x18, 0x03,
    0x55, 0x00, 0x53, 0x00, 0x42, 0x00, 0x2D, 0x00, 0x42, 0x00, 0x6C, 0x00,
    0x61, 0x00, 0x73, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x12, 0x03,
    0x43, 0x00, 0x30, 0x00, 0x42, 0x00, 0x46, 0x00, 0x41, 0x00, 0x36, 0x00,
    0x44, 0x00, 0x37, 0x00, 0x02, 0x03, 0x01, 0x00, 0x52, 0x45, 0x56, 0x42,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB5, 0xB2
};

/* 18 bytes  */
__code uint8_t DevDesc[] = {
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08,
    0xFB, 0x09, /* VID: 0x09FB */ 0x01, 0x60, /* PID: 0x6001 */
    0x00, 0x04, 0x01, 0x02, 0x03, 0x01
};

/* 32 bytes */
__code uint8_t CfgDesc[] = {
    0x09, 0x02, sizeof(CfgDesc) & 0xFF, sizeof(CfgDesc) >> 8, 0x01, 0x01, 0x00,
    0x80, 0xE1,
    /* Interface Descriptor */
    0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0xFF, 0xFF, 0x00,
    /* Endpoint Descriptor */
    0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x01, // EP1_IN
    0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x01, // EP2_OUT
};

/* USB String Descriptors (optional); 4 bytes */
unsigned char __code LangDes[] = { 0x04, 0x03, 0x09, 0x04 }; // EN_US

/* "USB-Blaster"; 24 bytes */
unsigned char __code ProdDes[] = {
    sizeof(ProdDes), 0x03, 'U', 0, 'S', 0, 'B', 0, '-', 0, 'B', 0, 'l', 0,
    'a', 0, 's', 0, 't', 0, 'e', 0, 'r', 0
};

/* "Altera"; 14 bytes */
unsigned char __code ManufDes[] = {
    sizeof(ManufDes), 0x03, 'A', 0, 'l', 0, 't', 0, 'e', 0, 'r', 0, 'a', 0
};

volatile __data uint8_t USBByteCount = 0; // Represents the data received by the USB endpoint
volatile __data uint8_t USBBufOutPoint = 0; // Get data pointer
volatile __data uint16_t sof_count = 0;
volatile __data uint8_t latency_timer = 4;

#define PENDING_ADDR (1 << 0)
#define SEND_DUMMY   (1 << 1)
#define EP1_IN_BUSY  (1 << 2) // The upload endpoint is busy

volatile __data uint8_t flags = SEND_DUMMY;

static inline void unlock(void) {
  SAFE_MOD = 0x55;
  SAFE_MOD = 0xAA;
}

static inline void sleep(void) {
#ifdef UART
  printf("sleep\n");
#endif

  while (XBUS_AUX & bUART0_TX); // Waiting for sending to complete

  unlock();
  // Can be woken up when USB or RXD0/1 has a signal.
  WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;

  PCON |= PD; // Sleep

  unlock();
  WAKE_CTRL = 0x00;
}

/*******************************************************************************
 * Function Name  : USBDeviceCfg()
 * Description  : Configure USB
 * Input      : None
 * Output    : None
 * Return    : None
 *******************************************************************************/
static inline void USBDeviceCfg(void) {
  // USB device and internal pull-up enabled, automatically returns NAK
  // during interrupt before interrupt flag is cleared
  USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;

  // Disable DP/DM pull-down resistor. Enable physical port
  UDEV_CTRL = bUD_PD_DIS | bUD_PORT_EN;
}

/*******************************************************************************
 * Function Name  : USBDeviceIntCfg()
 * Description  : USB device mode interrupt initialization
 * Input      : None
 * Output    : None
 * Return    : None
 *******************************************************************************/
static inline void USBDeviceIntCfg(void) {
  // Enable device hang interrupt. Enable USB transfer completion interrupt.
  // Enable device mode USB bus reset interrupt.
  USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST | bUIE_DEV_SOF;
  USB_INT_FG = 0x1F; // Clear interrupt flag
  IE_USB = 1;        // Enable USB interrupt
  EA = 1;            // Enable MCU interrupt
}

/*******************************************************************************
 * Function Name  : USBDeviceEndPointCfg()
 * Description  : USB device mode endpoint configuration, emulation compatible HID device,
 *      in addition to endpoint 0 control transmission, also includes endpoint 2 batch up and down transmission
 * Input      : None
 * Output    : None
 * Return    : None
 *******************************************************************************/
static inline void USBDeviceEndPointCfg(void) {
  // Endpoint 1 upload buffer. Endpoint 0 single 64-byte send and receive buffer.
  UEP4_1_MOD = bUEP1_TX_EN;
  UEP2_3_MOD = bUEP2_RX_EN;

  UEP0_DMA = (uint16_t)Ep0Buffer; // Endpoint 0 data transmission address
  UEP1_DMA = (uint16_t)Ep1Buffer; // Endpoint 1 IN data transmission address
  UEP2_DMA = (uint16_t)Ep2Buffer; // Endpoint 2 OUT data transmission address

  // Manual flip, OUT transaction returns ACK, IN transaction returns NAK
  UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
  // Endpoint 2 automatically flips the synchronization flag bit, and OUT
  // returns ACK
  UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
  // Endpoint 1 automatically flips the synchronization flag bit, and the IN
  // transaction returns NAK
  UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
}

/*******************************************************************************
 * Function Name : fastcpy()
 * Description   : custom memcopy function for __code and __xdata only
 * Input         : ptr for src and dest, len (8 Bit)
 * Output        : none 
 * Attention     : src and dest params are switched for more compact code
*******************************************************************************/
void fastcpy(void *src, void *dest, uint8_t len) { 
  dest; src; len;
   __asm
     mov    A,_fastcpy_PARM_3
     jnz    0$                      ; nothing todo -> exit
     ret
0$:
     push   ar7
     mov    R7,A                    ; Param 3 is loop counter
     push   _XBUS_AUX               ; save aux reg
     orl    _XBUS_AUX,#0x05         ; select DPTR1 + autoinc
     mov    DPL,_fastcpy_PARM_2+0   ; and load &dest to DPTR1
     mov    DPH,_fastcpy_PARM_2+1   ;
     dec    _XBUS_AUX               ; DPTR0 = *src
     mov    A,B                     ; 0x00 -> XDATA 
     JNZ    2$
1$:                                 ; xmem loop
     movx   A,@DPTR                 ; read source (xmem) with autoinc
     .db    0xA5                    ; MOVX @DPTR1,A & INC DPTR1
     djnz   R7,1$
     sjmp   3$                      ;done 
2$:                                 ; cmem loop 
     clr    A
     movc   A,@A+DPTR               ; read source (codemem)
     inc    DPTR                    ; src ++ no autoinc 
     .db    0xA5                    ; MOVX @DPTR1,A & INC DPTR1
     djnz   R7,2$    
3$:    pop    _XBUS_AUX             ; done
     pop    ar7
     ret
   __endasm;
}

/*
  Create a unique serial nummber based on CH552 UID
*/
static inline uint8_t nibble_to_hex(uint8_t n) {
  return n + (n < 10 ? '0' : 'A' - 10);
}

static inline void InitSerialNum(void) {
  uint8_t i, b;
  uint8_t volatile __code *src = ((uint8_t volatile __code *)ROM_CHIP_ID_HX);
  uint8_t *dst = SerialNum;

  *dst++ = sizeof(SerialNum);
  *dst++ = 0x03;

  for (i = 0; i < 5; i++) {
    b = *src++;
    *dst++ = nibble_to_hex(b >> 4);
    *dst++ = 0;
    *dst++ = nibble_to_hex(b & 0x0F);
    *dst++ = 0;
  }
}

/*
static inline uint8_t usb_req_recip_device_set(void) {
  uint16_t value = ((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL;

  if (value == 0x01 && CfgDesc[7] & 0x20) {
    sleep();
    return 0;
  }

  return 0xFF; // Operation failed
}
*/

uint8_t usb_req_recip_endp_set(void) {
  uint16_t value, index;

  value = ((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL;
  if (value != 0x00)
    return 0xFF; /* Operation failed */

  index = ((uint16_t)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL;
  switch (index) {
  case 0x82:
    /* Set endpoint 2 IN STALL */
    UEP2_CTRL = UEP2_CTRL & ~bUEP_T_TOG | UEP_T_RES_STALL;
    break;

  case 0x02:
    /* Set endpoint 2 OUT Stall */
    UEP2_CTRL = UEP2_CTRL & ~bUEP_R_TOG | UEP_R_RES_STALL;
    break;

  case 0x81:
    /* Set endpoint 1 IN STALL */
    UEP1_CTRL = UEP1_CTRL & ~bUEP_T_TOG | UEP_T_RES_STALL;
    break;

  case 0x01:
    /* Set endpoint 1 OUT Stall */
    UEP1_CTRL = UEP1_CTRL & ~bUEP_R_TOG | UEP_R_RES_STALL;
    break;

  default:
    return 0xFF; /* Operation failed */
  }

  return 0;
}

static inline uint8_t usb_req_typ_vendor(void) {
  switch (SetupReq) {
  case FTDI_VEN_REQ_RD_EEPROM: {
    uint16_t addr;

    addr = UsbSetupBuf->wIndexL << 1;
    if (addr >= sizeof(ftdi_rom) - 1)
      return 0xFF;

    Ep0Buffer[0] = ftdi_rom[addr];
    Ep0Buffer[1] = ftdi_rom[addr + 1];
    return 2;
  }

  case FTDI_VEN_REQ_GET_MODEM_STA:
    // Return fixed modem status
    Ep0Buffer[0] = FTDI_MODEM_STA_DUMMY0;
    Ep0Buffer[1] = FTDI_MODEM_STA_DUMMY1;
    return 2;

  case FTDI_VEN_REQ_SET_LAT_TIMER:
    latency_timer = UsbSetupBuf->wValueL;
    return 0;

  default:
    return 0xFF;
  }
}

static inline uint8_t copy_chunk(void) {
  uint8_t len = SetupLen;

  if (len > DEFAULT_ENDP0_SIZE)
    len = DEFAULT_ENDP0_SIZE;

  fastcpy(Descr, Ep0Buffer, len);
  SetupLen -= len;
  Descr += len;

  return len;
}

static inline uint8_t usb_get_descriptor(void) {
  uint8_t len;

  switch (UsbSetupBuf->wValueH) {
  case 1: // Device Descriptor
    Descr = DevDesc; // Send the device descriptor to the buffer to be sent
    len = sizeof(DevDesc);
    break;

  case 2: // Configuration Descriptor
    Descr = CfgDesc; // Send the device descriptor to the buffer to be sent
    len = sizeof(CfgDesc);
    break;

  case 3:
    if (UsbSetupBuf->wValueL > 3)
      return 0xFF; // Reject all strings exept id0..id3 unsupported string

    if (UsbSetupBuf->wValueL == 0) {
        Descr = LangDes;
        len = sizeof(LangDes);
    } else if (UsbSetupBuf->wValueL == 1) {
        Descr = ManufDes;
        len = sizeof(ManufDes);
    } else if (UsbSetupBuf->wValueL == 2) {
        Descr = ProdDes;
        len = sizeof(ProdDes);
    } else { /* if (UsbSetupBuf->wValueL == 3) */
        Descr = SerialNum;
        len = sizeof(SerialNum);
    }
    break;

  default:
    return 0xFF; // Unsupported command or error
  }

  // Limit total length
  if (SetupLen > len)
    SetupLen = len;

  return copy_chunk();
}

/*
static inline uint8_t usb_req_recip_device_clear(void) {
  uint16_t value = ((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL;

  if (value == 0x01 && CfgDesc[7] & 0x20) {
    // Wake up?
    return 0;
  }

  return 0xFF; // Operation failed
}
*/

static inline uint8_t usb_req_recip_endp_clear(void) {
  switch (UsbSetupBuf->wIndexL) {
  case 0x02:
    UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
    break;
  
  case 0x81:
    UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
    break;

  default:
    return 0xFF; // Unsupported endpoint
  }

  flags &= ~EP1_IN_BUSY;
  return 0;
}

static inline uint8_t usb_clear_feature(void) {
  uint8_t recipient = UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK;

/*
  if (recipient == USB_REQ_RECIP_DEVICE)
    return usb_req_recip_device_clear();
*/

  if (recipient == USB_REQ_RECIP_ENDP)
    return usb_req_recip_endp_clear();

  return 0xFF; // Not that the endpoint does not support
}

static inline uint8_t usb_set_feature(void) {
  uint8_t value = UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK;

/*
  if (value == USB_REQ_RECIP_DEVICE) // Setting up the device
    return usb_req_recip_device_set();
*/

  if (value == USB_REQ_RECIP_ENDP)   /* Setting up the endpoint */
    return usb_req_recip_endp_set();

  return 0xFF; /* Operation failed */
}

static inline uint8_t usb_get_status(void) {
  Ep0Buffer[0] = 0x00;
  Ep0Buffer[1] = 0x00;

  return SetupLen < 2 ? (uint8_t)SetupLen : 2;
}

static inline uint8_t usb_req_typ_standard(void) {
  switch (SetupReq) { // Request Code
  case USB_GET_DESCRIPTOR:
    return usb_get_descriptor();

  case USB_SET_ADDRESS:
    PendingAddr = (uint8_t)UsbSetupBuf->wValueL;
    flags |= PENDING_ADDR;
    break;

  case USB_GET_CONFIGURATION:
    Ep0Buffer[0] = UsbConfig;
    if (SetupLen >= 1)
      return 1;
    break;

  case USB_SET_CONFIGURATION:
    UsbConfig = UsbSetupBuf->wValueL;
    break;

  case USB_GET_INTERFACE:
    break;

  case USB_CLEAR_FEATURE: // Clear Feature
    return usb_clear_feature();

  case USB_SET_FEATURE: /* Set Feature */
    return usb_set_feature();

  case USB_GET_STATUS:
    return usb_get_status();

  default:
    return 0xFF; // Operation failed
  }

  return 0;
}

static inline uint8_t usb_setup_req(uint8_t len) {
  uint8_t request_type;

  if (len != sizeof(USB_SETUP_REQ))
    return 0xFF; // Packet length error

  SetupLen = ((uint16_t)UsbSetupBuf->wLengthH << 8) | UsbSetupBuf->wLengthL;
  SetupReq = UsbSetupBuf->bRequest;

  request_type = UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK;
  switch (request_type) {
  case USB_REQ_TYP_VENDOR:
    return usb_req_typ_vendor();

  case USB_REQ_TYP_STANDARD:
    return usb_req_typ_standard();

  default:
    return 0xFF; // Command not supported
  }
}

static inline void uis_token_ep0_in(void) {
  switch (SetupReq) {
  case USB_GET_DESCRIPTOR:
    if (SetupLen) {
      UEP0_T_LEN = copy_chunk();
      UEP0_CTRL ^= bUEP_T_TOG; // Synchronous flag bit flip
    } else {
      UEP0_T_LEN = 0;
      // Reset toggle, ACK
      UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_T_RES_ACK;
    }
    break;

  case USB_SET_ADDRESS:
    if (flags & PENDING_ADDR) {
      USB_DEV_AD |= PendingAddr;
      flags &= ~PENDING_ADDR;
    }
    break;

  default:
    // The status stage is completed and the interrupt is completed or
    // the zero-length data packet is forced to upload to end the control
    // transmission.
    UEP0_T_LEN = 0;
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    break;
  }
}

static inline void uif_bus_rst(void) {
#ifdef UART
  printf("uif_bus_rst\n");
#endif

  UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
  UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
  UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
  USB_DEV_AD = 0x00;
  USB_INT_FG = 0xFF;  // Clear interrupt flags

  USBByteCount = 0;   // The length received by the USB endpoint
  UsbConfig = 0;      // Clearing Configuration Values

  flags = SEND_DUMMY;
}

static inline void uif_suspend(void) {
  UIF_SUSPEND = 0;

  if (USB_MIS_ST & bUMS_SUSPEND) // Suspend
    sleep();
}

static inline void uis_token_in(void) {
  switch (USB_INT_ST & MASK_UIS_ENDP) {
  case 1:
    UEP1_T_LEN = 0;
    // Default response NAK
    UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;

    flags &= ~EP1_IN_BUSY;
    break;

  case 0: // endpoint0 IN
    uis_token_ep0_in();
    break;
  }
}

static inline void uis_token_out(void) {
  switch (USB_INT_ST & MASK_UIS_ENDP) {
  case 2:
    if (U_TOG_OK) { // Out-of-sync packets will be discarded
      USBByteCount = USB_RX_LEN;
      USBBufOutPoint = 0; // Reset data pointer

      // NAK is sent when a packet of data is received. After the main
      // function completes the processing, the main function modifies
      // the response mode.
      UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;
    }
    break;

  case 0:
    UEP0_T_LEN = 0;

    // Status phase, respond to IN with NAK
    UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;
    break;
  }
}

static inline void uis_token_setup(void) {
  uint8_t len;

  if ((USB_INT_ST & MASK_UIS_ENDP) != 0)
    return;

  // If the previous request was stalled this fixes a second stall
  UEP0_CTRL &= ~(bUEP_T_RES0 | bUEP_R_RES0 | bUEP_R_RES1);

  len = usb_setup_req(USB_RX_LEN);
  if (len == 0xFF) {
    // STALL
    SetupReq = 0xFF;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
  } else if (len <= DEFAULT_ENDP0_SIZE) {
    // Returns a 0-length packet during the data upload or status
    // upload phase.
    UEP0_T_LEN = len;

    // The default data packet is DATA1, and the response ACK is returned.
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
  }
}

static inline void uif_transfer(void) {
  switch (USB_INT_ST & MASK_UIS_TOKEN) {
  case UIS_TOKEN_SOF:
    if ((USB_INT_ST & MASK_UIS_ENDP) < 3)
       sof_count++;
    break;

  case UIS_TOKEN_IN:
    uis_token_in();
    break;

  case UIS_TOKEN_OUT:
    uis_token_out();
    break;

  case UIS_TOKEN_SETUP: // SETUP transaction
    uis_token_setup();
    break;
  }

  UIF_TRANSFER = 0; // Write 0 to clear the interrupt
}

/*******************************************************************************
 * Function Name  : DeviceInterrupt()
 * Description  : CH55xUSB interrupt processing function
 *******************************************************************************/
void DeviceInterrupt(void) __interrupt(INT_NO_USB) { // USB interrupt service routine, using register bank 1
  if (UIF_TRANSFER) // USB transfer completion flag
    uif_transfer();

  if (UIF_BUS_RST) // Device mode USB bus reset interrupt
    uif_bus_rst();

  if (UIF_SUSPEND) // USB bus suspend or wake up completed
    uif_suspend();
  else  // Unexpected interruption, impossible situation
    USB_INT_FG = 0xFF; // Clear interrupt flag
}

__data uint8_t transmit_buffer_in_offset;
__data uint8_t transmit_buffer_out_offset;
__data uint8_t send_len;

static inline uint8_t shift_data(void) {
  SPIMasterWrite(P2);
  return SPI0_DATA;
}

static inline void blink(uint8_t delay, uint8_t count) {
  uint8_t i;

  for (i = 0; i < count; i++) {
    LED = 1;
    delay_ms(delay);
    LED = 0;
    delay_ms(delay);
  }

  LED = 1;
}

static inline void read_buffer(uint8_t length) {
  uint8_t shift_en;
  uint8_t read_buffer_index = 0;
  uint8_t shift_count = 0;
  uint8_t read_en = 0;

  while (read_buffer_index < length) {
    P2 = receive_buffer[read_buffer_index];
    read_buffer_index++;

    // TODO: Assembly implementation for IO control.
    if (shift_count == 0) {
      SPI0_CTRL = 0;
      shift_en = P2B7;
      read_en = P2B6;

      if (shift_en) {
        shift_count = P2 & 0x3F;
        SPI0_CTRL = 0x60;
      } else if (read_en) {
        LED = !P2B5;
        TDI = P2B4;
        TMS = P2B1;
        TCK = P2B0;

        transmit_buffer[transmit_buffer_in_offset] = TDO;
        transmit_buffer_in_offset++;
        transmit_buffer_in_offset &= 0x7F;
      } else {
        LED = !P2B5;
        TDI = P2B4;
        TMS = P2B1;
        TCK = P2B0;
      }
  } else {
      shift_count--;
      
      if (read_en) {
        transmit_buffer[transmit_buffer_in_offset] = shift_data();
        transmit_buffer_in_offset++;
        transmit_buffer_in_offset &= 0x7F;
      } else
        shift_data();
    }
  }
}

void main(void)
{
  int8_t data_len;
  uint8_t i;
  uint8_t length = 0;
  uint16_t timeout_count = 0;

  CfgFsys();   // CH552 clock selection configuration
  delay_ms(5); // wait for clock become stabilize.

#ifdef UART
  UART1Setup();
#endif

  InitSerialNum();
  SPIMasterModeSetLSB();
  SPI_CK_SET(4);
  USBDeviceCfg();
  USBDeviceEndPointCfg(); // Endpoint configuration
  USBDeviceIntCfg();      // Interrupt initialization

  // P1.5, 1.7 output push-pull, P1.6 input
  P1_MOD_OC &= ~((1 << 5) | (1 << 7));
  P1_MOD_OC |= 1 << 6;
  P1_DIR_PU |= (1 << 5) | (1 << 6) | (1 << 7);

  // P3.1, 3.2 output push-pull
  P3_MOD_OC &= ~((1 << 1) | (1 << 2));
  P3_DIR_PU |= ((1 << 1) | (1 << 2));

  TDO = 1;

  UEP0_T_LEN = 0;
  UEP1_T_LEN = 0; // The pre-used sending length must be cleared

  Ep1Buffer[0] = FTDI_MODEM_STA_DUMMY0;
  Ep1Buffer[1] = FTDI_MODEM_STA_DUMMY1;

  transmit_buffer_in_offset = 0;
  transmit_buffer_out_offset = 0;

  blink(75, 4);

  while (1) {
    if (!UsbConfig)
        continue;

    if (USBByteCount) { // The USB receiving endpoint has data
      EA = 0;
      fastcpy(Ep2Buffer, receive_buffer, USBByteCount);
      EA = 1;

      UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
      length = USBByteCount;
      USBByteCount = 0;
    }

    read_buffer(length);

    if (flags & EP1_IN_BUSY)
      continue;

    // The endpoint is not busy (the first packet of data after being idle is only used to trigger upload)
    data_len = transmit_buffer_in_offset - transmit_buffer_out_offset;
    if (data_len < 0)
      data_len += 128;

    if (data_len > 0) {
      send_len = (data_len >= 62) ? 62 : data_len;

      for (i = 0; i < send_len; i++) {
        Ep1Buffer[i + 2] = transmit_buffer[transmit_buffer_out_offset];
        transmit_buffer_out_offset++;
        transmit_buffer_out_offset &= 0x7F;
      }

      flags |= EP1_IN_BUSY;
      UEP1_T_LEN = send_len + 2;
      UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK
    } else if ((sof_count - timeout_count) > latency_timer) {
      timeout_count = sof_count;
      flags |= EP1_IN_BUSY;
      UEP1_T_LEN = 2; // The pre-used sending length must be cleared
      UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK
    } else if (flags & SEND_DUMMY) {
      flags = flags & ~SEND_DUMMY | EP1_IN_BUSY;
      UEP1_T_LEN = 2; // The pre-used sending length must be cleared
      UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK
    }
  }
}
