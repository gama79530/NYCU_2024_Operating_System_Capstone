# Note

## Basic Exercise 1 - UART Bootloader

跟 c 共用就行，不需要另外製作。

## Basic Exercise 2 - Initial Ramdisk

## Basic Exercise 3 - Simple Allocator

定義 `MemoryManager` 在 `memory.hpp` 以及 `memory.cpp`。

## Advanced Exercise 1 - Bootloader Self Relocation

跟 c 共用就行，不需要另外製作。

## Advanced Exercise 2 - Devicetree

### 遇到了 undefined reference to `memset' 錯誤

```bash
rein@rein-Ubuntu24:~/workspace/NYCU_2024_Operating_System_Capstone/Lab2/cpp$ make
aarch64-linux-gnu-ld  -T src/linker.ld -o build/kernel8.elf  build/cstring_cpp.o build/device_tree_cpp.o build/kernel_cpp.o build/mailbox_cpp.o build/main_cpp.o build/memory_cpp.o build/mini_uart_cpp.o build/power_cpp.o build/printf_cpp.o build/shell_cpp.o build/util_cpp.o build/boot_s.o build/util_s.o -Map=build/kernel8.map
aarch64-linux-gnu-ld: build/kernel_cpp.o: in function `Kernel::Kernel(unsigned long)':
kernel.cpp:(.text+0x60): undefined reference to `memset'
make: *** [makefile:56: bin/kernel8.img] Error 1
```

可能是當 static 區塊使用的比較大塊的時候編譯器就會想要先使用 memset 來清空這個區塊。但不使用 std 就沒有現成的實做可以用。
最簡單的方式是自己實做一個 memset。 實做的時候要注意因為是用 cpp 實做所以需要加上 `extern "C"` 來讓 g++ 不會做額外包裝。
對於 header 裡面有 #ifdef __cplusplus 這個條件編譯則是提供更靈活的彈性，讓實做不只侷限在 cpp 而是也可以用 c 來實做。