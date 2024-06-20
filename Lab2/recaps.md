# Recaps

## [Basic Exercise 1 - UART Bootloader](https://nycu-caslab.github.io/OSC2024/labs/lab2.html#basic-exercise-1-uart-bootloader-30)

### remote_shell.py

This program works as a remote shell, communicating with a Raspberry Pi through UART. It sends request commands and receives response messages. Moreover, it provides functions that work with a PC client.

### loader

This program works as a proxy for the kernel. It provides simple shell functions and a command, **upload_kernel**, that downloads a kernel image from the PC client and loads it into the Raspberry Pi.

### kernel uploading protocol

1. **loader** sends a request string **'\$upload_kernel\$\n'** to *remote_shell*.
2. **remote_shell** receives **'\$upload_kernel\$\n'** and reples with **an integer string** that implies the size of kernel image.
3. **loader** receives the kernel size and replies with **'\$start_upload\n'**
4. **remote_shell** receives **'\$start_upload\n'** and upload **kernel image**
5. **loader** receives kernel image and replies with **'\$done\n'**

## Reference

1. [Lab 2: Booting](https://nycu-caslab.github.io/OSC2024/labs/lab2.html)
2. [bztsrc / raspi3-tutorial](https://github.com/bztsrc/raspi3-tutorial)
3. [s-matyukevich / raspberry-pi-os](https://github.com/s-matyukevich/raspberry-pi-os)
