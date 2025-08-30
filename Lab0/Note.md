# Note

## Cross-Platform Development

必須根據 `開發使用的平台` 以及 `佈署的目標平台` 去選擇使用的工具。
我使用的開發平台是 `Ubuntu 24.04`, 目標平台是 `Raspberry Pi 3b` 。

### Cross Compiler

```bash
# C
sudo apt install gcc-aarch64-linux-gnu
aarch64-linux-gnu-gcc --help

# C++
sudo apt install g++-aarch64-linux-gnu
aarch64-linux-gnu-g++ --help
```

### GDB

```bash
sudo apt install gdb-multiarch
gdb-multiarch --help
```

### QEMU

```bash
sudo apt install qemu-system-aarch64
qemu-system-aarch64 --help
```

## Firmware

Raspberry Pi 3b 要能執行需要
- Firmware for GPU.
- Kernel image (kernel8.img).

Kernel image 就是這個課程要製作的東西，而 firmware 可以從 [raspberrypi/firmware](https://github.com/raspberrypi/firmware/tree/master/boot) 找到，需要的檔案有 `bootcode.bin`, `fixup.dat` 以及 `start.elf` 。

Raspberry Pi 3B 的啟動流程是由內建的 **VideoCore IV GPU** 先上電並負責執行。GPU 上的 firmware 會進行開機流程，其中包含將 `kernel image` 載入到指定的記憶體位置。當這些動作完成後，控制權才會被交給 `kernel`，開始進入作業系統的執行階段。