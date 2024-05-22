# Recaps

## [Basic Exercise 1 - Basic Initialization](https://nycu-caslab.github.io/OSC2024/labs/lab1.html#basic-exercise-1-basic-initialization-20)

1. main.c
2. boot.S
3. linker.ld

## [Basic Exercise 2 - Mini UART](https://nycu-caslab.github.io/OSC2024/labs/lab1.html#basic-exercise-2-mini-uart-20)

1. mini_uart.h / mini_uart.c
2. gpio.h

## [Basic Exercise 3 - Simple Shell](https://nycu-caslab.github.io/OSC2024/labs/lab1.html#basic-exercise-3-simple-shell-20)

1. shell.h / shell.c
2. string.h / string.c

## [Basic Exercise 4 - Mailbox](https://nycu-caslab.github.io/OSC2024/labs/lab1.html#basic-exercise-4-mailbox-20)

1. mailbox.h / mailbox.c

## [Advanced Exercise 1 - Reboot](https://nycu-caslab.github.io/OSC2024/labs/lab1.html#advanced-exercise-1-reboot-30)

1. power.h / power.c
2. delays.h / delays.c

## Others

### GPFSEL1

The GPFSEL1 register is used to control alternative functions for pins 10-19.

### Alternative function

An alternative function is just a number from 0 to 5 that can be set for each pin and configures which device is connected to the pin.

### Mailbox

Mailbox is a communication mechanism between ARM and VideoCoreIV GPU. You can use it to set framebuffer or configure some peripherals.

## Reference

1. [Lab 1: Hello World](https://nycu-caslab.github.io/OSC2024/labs/lab1.html#)
2. [bztsrc / raspi3-tutorial](https://github.com/bztsrc/raspi3-tutorial)
3. [s-matyukevich / raspberry-pi-os](https://github.com/s-matyukevich/raspberry-pi-os)
4. [UART](https://nycu-caslab.github.io/OSC2024/labs/hardware/uart.html#uart)
5. [Mailbox](https://nycu-caslab.github.io/OSC2024/labs/hardware/mailbox.html)
6. [BCM2837 ARM Peripherals manual](https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf)
7. [浅谈ARM64汇编](https://leylfl.github.io/2018/05/15/%E6%B5%85%E8%B0%88ARM64%E6%B1%87%E7%BC%96/)
8. [raspberrypi/firmware - Mailboxes](https://github.com/raspberrypi/firmware/wiki/Mailboxes)
9. [Unit BCM2837](https://ultibo.org/wiki/Unit_BCM2837)
