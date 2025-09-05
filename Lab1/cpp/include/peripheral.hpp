#ifndef __PERIPHERAL_HPP__
#define __PERIPHERAL_HPP__

/*****************************************************************************************
 * MMIO 
 * ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 *      section 1.2.3, page 6
 *****************************************************************************************/
#define MMIO_BASE       0x3F000000

/*****************************************************************************************
 * GPIO 
 * ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf 
 *      section 6.1, page 90
 *****************************************************************************************/
#define GPIO_BASE       (MMIO_BASE + 0x00200000)

#define GPFSEL0         (GPIO_BASE + 0x00000000)
#define GPFSEL1         (GPIO_BASE + 0x00000004)
#define GPFSEL2         (GPIO_BASE + 0x00000008)
#define GPFSEL3         (GPIO_BASE + 0x0000000C)
#define GPFSEL4         (GPIO_BASE + 0x00000010)
#define GPFSEL5         (GPIO_BASE + 0x00000014)

#define GPSET0          (GPIO_BASE + 0x0000001C)
#define GPSET1          (GPIO_BASE + 0x00000020)

#define GPCLR0          (GPIO_BASE + 0x00000028)
#define GPCLR1          (GPIO_BASE + 0x0000002C)

#define GPLEV0          (GPIO_BASE + 0x00000034)
#define GPLEV1          (GPIO_BASE + 0x00000038)

#define GPEDS0          (GPIO_BASE + 0x00000040)
#define GPEDS1          (GPIO_BASE + 0x00000044)

#define GPHEN0          (GPIO_BASE + 0x00000064)
#define GPHEN1          (GPIO_BASE + 0x00000068)

#define GPLEN0          (GPIO_BASE + 0x00000070)
#define GPLEN1          (GPIO_BASE + 0x00000074)

#define GPAREN0         (GPIO_BASE + 0x0000007C)
#define GPAREN1         (GPIO_BASE + 0x00000080)

#define GPAFEN0         (GPIO_BASE + 0x00000088)
#define GPAFEN1         (GPIO_BASE + 0x0000008C)

#define GPPUD           (GPIO_BASE + 0x00000094)

#define GPPUDCLK0       (GPIO_BASE + 0x00000098)
#define GPPUDCLK1       (GPIO_BASE + 0x0000009C)

/*****************************************************************************************
 * Auxiliary peripherals
 * ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 *      section 2.1, page 8
 *****************************************************************************************/
#define AUX_BASE        (MMIO_BASE + 0x00215000)
/* Auxilary mini UART registers */
#define AUX_IRQ         (AUX_BASE + 0x00000000)
#define AUX_ENABLES     (AUX_BASE + 0x00000004) // Auxiliary enables            | 3
#define AUX_MU_IO       (AUX_BASE + 0x00000040) // I/O Data                     | 8
#define AUX_MU_IER      (AUX_BASE + 0x00000044) // Interrupt Enable Register    | 8
#define AUX_MU_IIR      (AUX_BASE + 0x00000048) // Interrupt Identify Register 	| 8
#define AUX_MU_LCR      (AUX_BASE + 0x0000004C) // Line Control Register        | 8
#define AUX_MU_MCR      (AUX_BASE + 0x00000050) // Modem Control Register       | 8
#define AUX_MU_LSR      (AUX_BASE + 0x00000054) // Line Status Register         | 8
#define AUX_MU_MSR      (AUX_BASE + 0x00000058) // Modem Status Register        | 8
#define AUX_MU_SCRATCH  (AUX_BASE + 0x0000005C) // Scratch                      | 8
#define AUX_MU_CNTL     (AUX_BASE + 0x00000060) // Extra Control                | 8
#define AUX_MU_STAT     (AUX_BASE + 0x00000064) // Extra Status                 | 32
#define AUX_MU_BAUD     (AUX_BASE + 0x00000068) // Baudrate                     | 16

#define AUX_SPI0_CNTL0  (AUX_BASE + 0x00000080)
#define AUX_SPI0_CNTL1  (AUX_BASE + 0x00000084)
#define AUX_SPI0_STAT   (AUX_BASE + 0x00000088)
#define AUX_SPI0_IO     (AUX_BASE + 0x00000090)
#define AUX_SPI0_PEEK   (AUX_BASE + 0x00000094)
#define AUX_SPI1_CNTL0  (AUX_BASE + 0x000000C0)
#define AUX_SPI1_CNTL1  (AUX_BASE + 0x000000C4)
#define AUX_SPI1_STAT   (AUX_BASE + 0x000000C8)
#define AUX_SPI1_IO     (AUX_BASE + 0x000000D0)
#define AUX_SPI1_PEEK   (AUX_BASE + 0x000000D4)

/*****************************************************************************************  
 * MAILBOX
 * ref: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface    
 ******************************************************************************************/
#define MBOX_BASE       (MMIO_BASE + 0x0000B880)

#define MBOX_READ       (MBOX_BASE + 0x00000000)
#define MBOX_POLL       (MBOX_BASE + 0x00000010)
#define MBOX_SENDER     (MBOX_BASE + 0x00000014)
#define MBOX_STATUS     (MBOX_BASE + 0x00000018)
#define MBOX_CONFIG     (MBOX_BASE + 0x0000001C)
#define MBOX_WRITE      (MBOX_BASE + 0x00000020)

/*
Mailbox property interface
ref: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface

Buffer contents:
    u32: size in bytes, (x+1) * 4, including the end tag
    u32: Request code / response code
    u8 ...: sequence of concatenated tags
    u32: the end tag
    u8 ...: padding

Tag format:
    u32: tag identifier
    u32: value buffer size in bytes
    u32: request code / response code
    b31 = 0: request, b30-b0: reserved
    b31 = 1: response, b30-b0: value length in bytes
    u8 ...: value buffer
    u8 ...: padding to align to 32 bits
*/
#define MBOX_EMPTY                  0x40000000
#define MBOX_FULL                   0x80000000

#define MBOX_REQUEST                0x00000000
#define MBOX_RESPONSE               0x80000000
#define MBOX_RESPONSE_SUCCEED       0x80000000
#define MBOX_RESPONSE_FAILED        0x80000001

/* channels */
#define MBOX_CH_POWER               0
#define MBOX_CH_FB                  1
#define MBOX_CH_VUART               2
#define MBOX_CH_VCHIQ               3
#define MBOX_CH_LEDS                4
#define MBOX_CH_BTNS                5
#define MBOX_CH_TOUCH               6
#define MBOX_CH_COUNT               7
#define MBOX_CH_PROP                8               // we only use channel 8 (CPU->GPU)

/* tags */
#define MBOX_TAG_GETFWREVISION      0x00000001      // u32: firmware revision 
#define MBOX_TAG_GETMODEL           0x00010001      // u32: board model
#define MBOX_TAG_GETREVISION        0x00010002      // u32: board revision
#define MBOX_TAG_GETMACADDR         0x00010003      // u8 * 6: MAC address in network byte order
#define MBOX_TAG_GETSERIAL          0x00010004      // u64: board serial
#define MBOX_TAG_GETMEMORY          0x00010005      // u32: base, u32: size
#define MBOX_TAG_GETVCMEM           0x00010006      // u32: base, u32: size
#define MBOX_TAG_GETCLOCKS          0x00010007      // u32: parent clock id, u32: clock id, (repeated)

#define MBOX_TAG_ALLOCATEFB         0x00040001      // (u32: alignment in bytes) -> u32: base address, u32: size

#define MBOX_TAG_GETPHYSICALWH      0x00040003      // u32: width in pixels, u32: height in pixels
#define MBOX_TAG_GETVIRTUALWH       0x00040004      // u32: width in pixels, u32: height in pixels
#define MBOX_TAG_GETDEPTH           0x00040005      // u32: bits per pixel
#define MBOX_TAG_GETPIXELORDER      0x00040006      // u32: 0x0=BGR, 0x1=RGB
#define MBOX_TAG_GETALPHAMODE       0x00040007      // u32: 0x0=enalbed, 0x1=reversed, 0x2=ignored
#define MBOX_TAG_GETPITCH           0x00040008      // u32: bytes per line
#define MBOX_TAG_GETVIRTUALOFFSET   0x00040009      // u32: X in pixels, u32: Y in pixels
#define MBOX_TAG_GETOVERSCAN        0x0004000a  
#define MBOX_TAG_GETPALETTE         0x0004000b

#define MBOX_TAG_REQUEST            0x00000000
#define MBOX_TAG_END                0x00000000

/*****************************************************************************************
 * POWER
 *****************************************************************************************/
#define PM_RSTC         (MMIO_BASE + 0x0010001c)
#define PM_RSTS         (MMIO_BASE + 0x00100020)
#define PM_WDOG         (MMIO_BASE + 0x00100024)

#endif
