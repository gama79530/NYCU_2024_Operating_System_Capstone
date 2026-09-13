#ifndef LAB5_C_PERIPHERAL_H
#define LAB5_C_PERIPHERAL_H

/*****************************************************************************************
 * MMIO
 * ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 *      section 1.2.3, page 6
 *****************************************************************************************/
#define MMIO_BASE 0x3F000000

/*****************************************************************************************
 * GPIO
 * ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 *      section 6.1, page 90
 *****************************************************************************************/
#define GPIO_BASE (MMIO_BASE + 0x00200000)

#define GPFSEL0 (GPIO_BASE + 0x00000000)
#define GPFSEL1 (GPIO_BASE + 0x00000004)
#define GPFSEL2 (GPIO_BASE + 0x00000008)
#define GPFSEL3 (GPIO_BASE + 0x0000000C)
#define GPFSEL4 (GPIO_BASE + 0x00000010)
#define GPFSEL5 (GPIO_BASE + 0x00000014)

#define GPSET0 (GPIO_BASE + 0x0000001C)
#define GPSET1 (GPIO_BASE + 0x00000020)

#define GPCLR0 (GPIO_BASE + 0x00000028)
#define GPCLR1 (GPIO_BASE + 0x0000002C)

#define GPLEV0 (GPIO_BASE + 0x00000034)
#define GPLEV1 (GPIO_BASE + 0x00000038)

#define GPEDS0 (GPIO_BASE + 0x00000040)
#define GPEDS1 (GPIO_BASE + 0x00000044)

#define GPHEN0 (GPIO_BASE + 0x00000064)
#define GPHEN1 (GPIO_BASE + 0x00000068)

#define GPLEN0 (GPIO_BASE + 0x00000070)
#define GPLEN1 (GPIO_BASE + 0x00000074)

#define GPAREN0 (GPIO_BASE + 0x0000007C)
#define GPAREN1 (GPIO_BASE + 0x00000080)

#define GPAFEN0 (GPIO_BASE + 0x00000088)
#define GPAFEN1 (GPIO_BASE + 0x0000008C)

#define GPPUD (GPIO_BASE + 0x00000094)

#define GPPUDCLK0 (GPIO_BASE + 0x00000098)
#define GPPUDCLK1 (GPIO_BASE + 0x0000009C)

/*****************************************************************************************
 * Auxiliary peripherals
 * ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 *      section 2.1, page 8
 *****************************************************************************************/
#define AUX_BASE (MMIO_BASE + 0x00215000)
/* Auxilary mini UART registers */
#define AUX_IRQ (AUX_BASE + 0x00000000)
#define AUX_ENABLES (AUX_BASE + 0x00000004)     // Auxiliary enables            | 3
#define AUX_MU_IO (AUX_BASE + 0x00000040)       // I/O Data                     | 8
#define AUX_MU_IER (AUX_BASE + 0x00000044)      // Interrupt Enable Register    | 8
#define AUX_MU_IIR (AUX_BASE + 0x00000048)      // Interrupt Identify Register 	| 8
#define AUX_MU_LCR (AUX_BASE + 0x0000004C)      // Line Control Register        | 8
#define AUX_MU_MCR (AUX_BASE + 0x00000050)      // Modem Control Register       | 8
#define AUX_MU_LSR (AUX_BASE + 0x00000054)      // Line Status Register         | 8
#define AUX_MU_MSR (AUX_BASE + 0x00000058)      // Modem Status Register        | 8
#define AUX_MU_SCRATCH (AUX_BASE + 0x0000005C)  // Scratch                      | 8
#define AUX_MU_CNTL (AUX_BASE + 0x00000060)     // Extra Control                | 8
#define AUX_MU_STAT (AUX_BASE + 0x00000064)     // Extra Status                 | 32
#define AUX_MU_BAUD (AUX_BASE + 0x00000068)     // Baudrate                     | 16

#define AUX_SPI0_CNTL0 (AUX_BASE + 0x00000080)
#define AUX_SPI0_CNTL1 (AUX_BASE + 0x00000084)
#define AUX_SPI0_STAT (AUX_BASE + 0x00000088)
#define AUX_SPI0_IO (AUX_BASE + 0x00000090)
#define AUX_SPI0_PEEK (AUX_BASE + 0x00000094)
#define AUX_SPI1_CNTL0 (AUX_BASE + 0x000000C0)
#define AUX_SPI1_CNTL1 (AUX_BASE + 0x000000C4)
#define AUX_SPI1_STAT (AUX_BASE + 0x000000C8)
#define AUX_SPI1_IO (AUX_BASE + 0x000000D0)
#define AUX_SPI1_PEEK (AUX_BASE + 0x000000D4)

#define AUX_IRQ_MINI_UART        (1 << 0)
#define AUX_MU_IER_RX            (1 << 0)
#define AUX_MU_IER_TX            (1 << 1)
#define AUX_MU_IIR_NO_INTERRUPT  (1 << 0)
#define AUX_MU_IIR_ID_MASK       (3 << 1)
#define AUX_MU_IIR_ID_TX         (1 << 1)
#define AUX_MU_IIR_ID_RX         (2 << 1)
#define AUX_MU_IIR_ID_RX_TIMEOUT (3 << 1)
#define AUX_MU_LSR_DATA_READY    (1 << 0)
#define AUX_MU_LSR_TX_READY      (1 << 5)

/*****************************************************************************************
 * MAILBOX
 * ref: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 ******************************************************************************************/
#define MAILBOX_BASE (MMIO_BASE + 0x0000B880)

#define MAILBOX_READ (MAILBOX_BASE + 0x00000000)
#define MAILBOX_POLL (MAILBOX_BASE + 0x00000010)
#define MAILBOX_SENDER (MAILBOX_BASE + 0x00000014)
#define MAILBOX_STATUS (MAILBOX_BASE + 0x00000018)
#define MAILBOX_CONFIG (MAILBOX_BASE + 0x0000001C)
#define MAILBOX_WRITE (MAILBOX_BASE + 0x00000020)

#define MAILBOX_STATUS_EMPTY 0x40000000
#define MAILBOX_STATUS_FULL 0x80000000

/*****************************************************************************************
 * POWER
 *****************************************************************************************/
#define PM_BASE (MMIO_BASE + 0x00100000)

#define PM_RSTC (PM_BASE + 0x0000001C)
#define PM_RSTS (PM_BASE + 0x00000020)
#define PM_WDOG (PM_BASE + 0x00000024)

/*****************************************************************************************
 * Core local interrupt controller
 *****************************************************************************************/
#define CORE_TIMER_IRQ_CTRL_CNTPNSIRQ (1 << 1)

#define CORE0_TIMER_IRQ_CTRL 0x40000040
#define CORE0_IRQ_SOURCE     0x40000060

/*****************************************************************************************
 * BCM2837 interrupt controller
 *****************************************************************************************/
#define IRQ_BASE (MMIO_BASE + 0x0000B000)

#define IRQ_PENDING_1 (IRQ_BASE + 0x00000204)
#define ENABLE_IRQS_1 (IRQ_BASE + 0x00000210)

#define IRQ_AUX (1 << 29)

#endif
