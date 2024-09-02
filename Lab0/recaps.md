# Recaps

## Description

The following command is how to execute this Lab 0. You can find more explanation from [here](https://gama79530.github.io/3_Course/OperatingSystemCapstone/Lab0/index.html).

```bash
# install cross compiler on Ubuntu.
sudo apt install gcc-aarch64-linux-gnu

# install QEMU
sudo apt install qemu-system-aarch64

# install Cross Plateform GDB
sudo apt install gdb-multiarch

# using QEMU to see the dumped assembly
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

# from Object Files to ELF
aarch64-linux-gnu-ld -T linker.ld -o kernel8.elf a.o

# from Source Code to Object Files
aarch64-linux-gnu-gcc -c a.S

# from ELF to Kernel Image
aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

# execute kernel image with debug mode on QEMU
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s -serial null -serial stdio

# interact with Rpi3
sudo screen /dev/ttyUSB0 115200
```
