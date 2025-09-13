#ifndef KERNEL_LOADER_PERIPHERAL_H
#define KERNEL_LOADER_PERIPHERAL_H

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
 * POWER
 *****************************************************************************************/
#define PM_RSTC         (MMIO_BASE + 0x0010001c)
#define PM_RSTS         (MMIO_BASE + 0x00100020)
#define PM_WDOG         (MMIO_BASE + 0x00100024)

#endif
