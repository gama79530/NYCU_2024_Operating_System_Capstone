# Recaps

## [Basic Exercise 1 - UART Bootloader](https://nycu-caslab.github.io/OSC2024/labs/lab2.html#basic-exercise-1-uart-bootloader-30)

### uploader.py

This program functions as a remote shell, communicating with a Raspberry Pi via UART. It sends request commands and receives response messages. Additionally, it provides functions that work with a PC client.

### loader

This program acts as a proxy for the kernel. It provides basic shell functions and includes two commands: **"download"**, which downloads a kernel image from the PC client, and **"boot"**, which loads the downloaded image onto the Raspberry Pi.

### kernel uploading protocol

1. **uploader** sends a command *"download\n"* to **loader**.
2. **loader** initiates the download process by sending a request string *"\$upload_kernel\$\n"* to **uploader**.
3. **uploader** receives *"\$upload_kernel\$\n"* and replies with a *"4-byte data"* that indicates the size of the kernel image in little-endian format.
4. **loader** receives the kernel size and replies with *"start_upload\$"*.
5. **uploader** receives *"start_upload\$"* and uploads the kernel image.
6. **loader** receives the kernel image and replies with *"done\$"*.
7. **uploader** receives *"done\$"* and ends the process.

## [Basic Exercise 2 - Initial Ramdisk](https://nycu-caslab.github.io/OSC2024/labs/lab2.html#basic-exercise-2-initial-ramdisk-30)

1. cpio.h / cpio.c

## [Basic Exercise 3 - Simple Allocator](https://nycu-caslab.github.io/OSC2024/labs/lab2.html#basic-exercise-3-simple-allocator-10)

1. memory.h / memory.c

## [Advanced Exercise 1 - Bootloader Self Relocation](https://nycu-caslab.github.io/OSC2024/labs/lab2.html#advanced-exercise-1-bootloader-self-relocation-10)

1. Load the loader at address 0x80000.
2. Set the offset to 0x20000.
3. Copy the code to address 0x60000 (0x80000 - offset).
4. Jump to the offset main function of the loader.

## [Advanced Exercise 2 - Devicetree](https://nycu-caslab.github.io/OSC2024/labs/lab2.html#advanced-exercise-2-devicetree-30)

1. Read the format of flatten format tree.

## Reference

1. [Lab 2: Booting](https://nycu-caslab.github.io/OSC2024/labs/lab2.html)
2. [bztsrc / raspi3-tutorial](https://github.com/bztsrc/raspi3-tutorial)
3. [s-matyukevich / raspberry-pi-os](https://github.com/s-matyukevich/raspberry-pi-os)
4. [Linker Script初探 - GNU Linker Ld手冊略讀](https://wen00072.github.io/blog/2014/03/14/study-on-the-linker-script/#bkg-sec)
5. [FreeBSD Manual Pages](https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5)
6. [RPiconfig](https://elinux.org/RPiconfig)
7. [Device Tree](https://hackmd.io/@0xff07/r1cJFN4QD)
