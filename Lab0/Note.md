# Lab 0 Note

Lab 0 的重點是建立 Raspberry Pi 3 bare-metal 開發環境，並練習從
assembly source code 產生可以被 Raspberry Pi firmware 載入的
`kernel8.img`。

## Lab 目標

- 安裝 cross-platform development 需要的工具。
- 理解 host platform 與 target platform 不同時，為什麼需要 cross compiler。
- 產生第一個 AArch64 kernel image。
- 使用 QEMU 檢查 kernel image 是否能被載入。
- 準備 Raspberry Pi 3 的 boot partition 並測試 UART 連線。

## 開發環境

目前使用的環境：

- Host platform: Ubuntu 24.04
- Target platform: Raspberry Pi 3B
- Target architecture: ARMv8-A / AArch64

Raspberry Pi 3 使用 ARM Cortex-A53 CPU。如果 host 不是 AArch64 環境，就需要
cross compiler 產生 target 可以執行的 64-bit ARM machine code。

## 工具安裝

### Cross Compiler

```bash
sudo apt install gcc-aarch64-linux-gnu
aarch64-linux-gnu-gcc --help
```

如果之後需要 C++：

```bash
sudo apt install g++-aarch64-linux-gnu
aarch64-linux-gnu-g++ --help
```

### QEMU

QEMU 可以先在 host 上模擬 Raspberry Pi 3，方便檢查 kernel image 是否能載入。
不過 QEMU 和真實硬體仍可能有差異，最後仍要在 Raspberry Pi 3 上驗證。

```bash
sudo apt install qemu-system-aarch64
qemu-system-aarch64 --help
```

### GDB

bare-metal kernel 沒有 host OS 可以幫忙 debug，因此常用 QEMU 搭配
`gdb-multiarch` 進行遠端除錯。

```bash
sudo apt install gdb-multiarch
gdb-multiarch --help
```

## Raspberry Pi 3 開機流程

Raspberry Pi 3 上電後，會先由 VideoCore IV GPU 執行 firmware。Firmware 會
初始化必要的硬體狀態，並從 boot partition 載入 kernel image。對 64-bit kernel
來說，預設載入的檔名是 `kernel8.img`。

一個可以開機的 FAT16/32 boot partition 至少需要：

- GPU firmware
- Kernel image: `kernel8.img`

課程需要的 firmware 可以從
[raspberrypi/firmware](https://github.com/raspberrypi/firmware/tree/master/boot)
取得，Lab 0 需要的基本檔案有：

- `bootcode.bin`
- `fixup.dat`
- `start.elf`

## Source Code 到 Kernel Image

Lab 0 的最小 kernel 只會進入等待事件的無限迴圈：

```asm
.section ".text"

_start:
  wfe
  b _start
```

`wfe` 是 wait for event，會讓 CPU 進入等待狀態；下一行再跳回 `_start`，
形成最小可執行迴圈。

### 1. Source Code 到 Object File

```bash
cd Lab0/c
aarch64-linux-gnu-gcc -c a.S
```

這會把 `a.S` 組譯成 AArch64 relocatable object file：`a.o`。

如果是在 C++ 工具鏈環境中，也可以用 `g++` driver 組譯同一份 assembly：

```bash
cd Lab0/cpp
aarch64-linux-gnu-g++ -c a.S
```

這個 Lab 的 `a.S` 是純 assembly，使用 `gcc` 或 `g++` driver 的結果在這裡沒有
本質差異；差別主要是之後連結 C++ object files 時，`g++` 會自動帶入 C++ 相關
runtime/linker 行為。不過 bare-metal kernel 通常會避免依賴 host runtime，所以
Lab 0 仍以明確的 `ld` 和 linker script 連結。

### 2. Object File 到 ELF

Bare-metal kernel 需要自己決定記憶體 layout。Lab 0 的 linker script 先把
`.text` section 放在 `0x80000`：

```ld
SECTIONS
{
  . = 0x80000;
  .text : { *(.text) }
}
```

連結指令：

```bash
aarch64-linux-gnu-ld -T linker.ld -o kernel8.elf a.o
```

`kernel8.elf` 保留 ELF metadata、symbol table 與 debug information，適合拿來
給 GDB 使用。

### 3. ELF 到 Raw Binary Image

Raspberry Pi firmware 不會直接載入 ELF，因此需要把 ELF 轉成 raw binary：

```bash
aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img
```

`kernel8.img` 才是要放進 boot partition 或丟給 QEMU 的 kernel image。

## 使用 QEMU 檢查

產生 `kernel8.img` 後，可以用 QEMU 載入並 dump instruction：

```bash
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm
```

如果載入成功，輸出中應該會看到 kernel 被放到 `0x00080000` 附近，並執行
`wfe` 與跳回 `_start` 的 branch instruction。

也可以用工具直接檢查 ELF：

```bash
aarch64-linux-gnu-readelf -h kernel8.elf
aarch64-linux-gnu-objdump -d kernel8.elf
```

重點是確認：

- Entry point address 是 `0x80000`。
- `_start` 位在 `0x80000`。
- 反組譯內容是 `wfe` 和 `b _start`。

## 部署到 Raspberry Pi 3

### 準備 SD Card

Lab 0 spec 提供兩種方式：

1. 使用課程提供的 bootable image，直接寫入 SD card。
2. 自己建立 FAT32 boot partition，並放入 firmware 和 `kernel8.img`。

如果使用 `dd`，務必先用 `lsblk` 確認 SD card 的 device path：

```bash
lsblk
sudo dd if=nycuos.img of=/dev/sdX
```

`/dev/sdX` 要換成實際 SD card 裝置。這個指令會覆蓋目標磁碟，不能填錯。

如果自己建立 partition，除了用 `mkfs.fat -F 32` 建立 FAT32 filesystem，也要
確認 partition type 設為 FAT。

### UART 測試

Lab 0 的實機測試會透過 UART 確認 Raspberry Pi 3 和 host 可以互動。

連接 UART to USB converter 時，需要接：

- TX
- RX
- GND

開啟 serial console：

```bash
sudo screen /dev/ttyUSB0 115200
```

如果使用課程提供的 echo kernel image，輸入字元後 serial console 應該會印出
相同字元。

## QEMU Debug

Debug 版本建議加上 `-g`，讓 object file 包含 debug information：

```bash
cd Lab0/c
aarch64-linux-gnu-gcc -g -c a.S
aarch64-linux-gnu-ld -T linker.ld -o kernel8.elf a.o
aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img
```

如果使用 `cpp` 目錄的版本，第一行組譯也可以換成：

```bash
cd Lab0/cpp
aarch64-linux-gnu-g++ -g -c a.S
```

第一個 terminal 啟動 QEMU，並等待 GDB 連線：

```bash
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s
```

第二個 terminal 啟動 GDB：

```bash
cd Lab0/c
gdb-multiarch kernel8.elf
```

進入 GDB 後連到 QEMU：

```gdb
target remote :1234
```

如果啟動 GDB 時沒有帶入 ELF，也可以在 GDB 內載入：

```gdb
file kernel8.elf
target remote :1234
```

## GDB 常用指令

### 斷點

```gdb
# 設定 breakpoint
b <locator>

# 刪除 breakpoint
d <locator>

# 顯示目前所有 breakpoints
info b

# 啟用或停用 breakpoint
enable <breakpoint>
disable <breakpoint>
```

`<locator>` 可以是記憶體位置、行號或 symbol name。

### 執行控制

```gdb
# 繼續執行
c

# 單步執行一條 instruction
si

# 單步執行到下一行 source
n

# 進入函式
s

# 執行到指定位置
until <locator>
```

### 狀態檢查

```gdb
# 顯示 registers
info registers

# 顯示目前 PC 指向的 instruction
x/i $pc

# 顯示一段 instruction
x/8i $pc

# 印出暫存器或變數
p /x $pc
p /x $sp

# 每次暫停時自動顯示
display /i $pc
display /x $sp
```

### TUI

```gdb
layout src
layout asm
tui disable
```

也可以用 `Ctrl-x a` 切換 TUI 模式。

### Print 格式

| 格式 | 功能 | 範例 |
| --- | --- | --- |
| `/d` | 十進位 | `p /d x` |
| `/x` | 十六進位 | `p /x x` |
| `/o` | 八進位 | `p /o x` |
| `/t` | 二進位 | `p /t x` |
| `/f` | 浮點數 | `p /f fval` |
| `/s` | 字串 | `p /s ptr` |
| `/a` | 地址 | `p /a var` |
| `/i` | instruction | `x/i $pc` |
| `/xb` | 以 byte 顯示記憶體 | `x/16xb $sp` |
| `/xw` | 以 word 顯示記憶體 | `x/8xw $sp` |

## Lab 0 Checklist

- 已安裝 `gcc-aarch64-linux-gnu`。
- 已安裝 `qemu-system-aarch64`。
- 已安裝 `gdb-multiarch`。
- 可以從 `a.S` build 出 `a.o`、`kernel8.elf`、`kernel8.img`。
- QEMU 可以載入 `kernel8.img`，並 dump 出 `_start` 的 instructions。
- SD card boot partition 已準備 firmware 和 `kernel8.img`。
- UART 可以用 `screen /dev/ttyUSB0 115200` 連線測試。
