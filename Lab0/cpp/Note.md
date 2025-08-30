# Note

# 基本指令

```bash
# 將 a.S 編譯成 object file
aarch64-linux-gnu-g++ -c a.S

# 將 object file link 成 ELF
aarch64-linux-gnu-ld -T linker.ld -o kernel8.elf a.o

# 將 ELF 打包成 image
aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

# 使用 QEMU 去查看 dumped assembly code
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

# 佈署到 Raspberry Pi 上面用 screen 去執行
sudo screen /dev/ttyUSB0 115200
```

## Debug

要 debug 有以下要點

- 編譯時要多加一個 `-g` 的參數
- 總共要使用兩個 terminal 依序分別操作
  - 用 QEMU 搭配特定參數讓 gdb 可以跟 QEMU 對接
  - 啟動 GDB 開始，進入 GDB 後連到事先開啟的 QEMU 進行 debug

```bash
# 編譯指令 
aarch64-linux-gnu-g++ -g -c a.S

# for debug
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s -serial null -serial stdio

# 啟動 gdb
gdb-multiarch kernel8.elf
```

### GDB 常用指令

#### 基本設定

```bash
# 連接到 QEMU 執行的 process
target remote :1234

# 另外載入執行檔
file ./kernel8.elf
```

#### 斷點設定

```bash
# set breakpoint
b <locator>
# delete breakpoint, <locator> 可以是 記憶體位置, line num, function name, 不指定的話會刪光現有的所有 breakpoints
d <locator>
# 顯示目前所有的 breakpoints
info b
# 啟用 breakpoint
enable <breakpoint>
# 停用 breakpoint
disable <breakpoint>
```
※ <locator> 可以是 `記憶體位置`, `line num`, `function 名稱`

#### 執行控制

```bash
# 執行程式 
r
# 繼續執行
c
# 單步執行 (跳過函式)
n
# 單步執行 (進入函式)
s
# 執行到特定位置, <locator> 不指定的話會刪光現有的所有 breakpoints
until <locator>
```

※ <locator> 可以是 `記憶體位置`, `line num`, `function 名稱`

#### 狀態監控

```bash
# TUI 模式開關
layout src
layout asm
tui disable # 或使用 ctrl+x a

# 設定變數
set <arg> = <value>
# 印出變數
p <format> <var>
# 每次暫停自動印出變數
display <format> <var>
# 查看所有區域變數
info locals
# 查看所有 registers
info registers
# 查看 stack
bt
```
※ <arg> 通常會用 $<arg_name> 表示
※ <format> 是指定輸出格式，也可以不指定
※ <var> 可以是 `特定記憶體位置`, `設定好的變數`, `程式內的特定變數`, `特定 register`
※ 常用格式表:
| 格式 | 功能 | 範例 |
|------|------|------|
| /d   | 十進位（decimal） | `p /d x` |
| /x   | 十六進位（hex） | `p /x x` |
| /o   | 八進位（octal） | `p /o x` |
| /t   | 二進位（binary） | `p /t x` |
| /f   | 浮點數（float） | `p /f fval` |
| /s   | 字串（string） | `p /s ptr` |
| /a   | 地址（address） | `p /a var` |
| /i   | 指令（instruction，針對地址） | `p /i $pc` |
| /xb  | 十六進位顯示位元組（byte） | `p /16xb &buffer` |
| /xw  | 十六進位顯示字（word） | `p /8xw &buffer` |
