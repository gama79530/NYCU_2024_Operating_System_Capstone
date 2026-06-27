# Lab 2 Note

Lab 2 的目標是把 Lab 1 的互動式 kernel 延伸到更完整的 booting 流程。這次會先做
一個透過 UART 載入 kernel image 的 bootloader，接著讓 kernel 能讀取 initial
ramdisk 裡的檔案，並加入 early boot 階段可用的 simple allocator。進階部分則包含
bootloader self relocation 與 flattened devicetree parser。

## 實作總覽

建議把 Lab 2 拆成這幾個項目完成：

| 項目 | 主要任務 | 可能檔案 |
| --- | --- | --- |
| UART bootloader | 透過 UART 接收 kernel image，寫入目標位址後跳轉 | `boot.S`, `main.c`, `mini_uart.*`, `linker.ld` |
| Kernel loading config | 讓 firmware 載入 bootloader 到不會和 kernel 重疊的位置 | `config.txt`, `linker.ld`, `makefile` |
| Initial ramdisk | 建立 cpio archive，kernel 解析 newc 格式並讀取檔案內容 | `cpio.*`, `shell.c`, `makefile`, `rootfs/` |
| Simple allocator | 在 early boot 階段提供只配置、不釋放的連續記憶體配置器 | `allocator.*`, `linker.ld`, `config.h` |
| Bootloader relocation | bootloader 啟動後搬移自己，避免與 kernel loading address 重疊 | `boot.S`, `linker.ld`, `main.c` |
| Devicetree | 解析 FDT，遍歷 nodes/properties，從 dtb 取得 initramfs 位址 | `fdt.*`, `boot.S`, `main.c` |
| Build and run | 分別建置 bootloader、kernel、initramfs，使用 QEMU 和實機驗證 | `makefile`, `config.txt` |

建議實作順序：

1. 先確認 Lab 1 kernel 在 Lab 2 目錄中仍可 build/run。
2. 拆出 bootloader image 和 actual kernel image，確認 linker address 不互相重疊。
3. 完成 UART bootloader 的傳輸協定、接收流程與跳轉。
4. 建立 `initramfs.cpio`，先用 hardcoded address 解析 newc archive。
5. 加入 shell command 來列出檔案與讀取指定檔案內容。
6. 實作 simple allocator，將需要 early allocation 的資料改成透過它取得。
7. 進階再補 bootloader self relocation。
8. 最後解析 devicetree，改由 dtb 取得 initramfs 載入範圍。

## UART Bootloader

Lab 2 的 basic exercise 1 要實作一個被 Raspberry Pi firmware 載入的 bootloader。
這個 bootloader 本身不是最終 kernel，而是透過 UART 從 host 接收真正要執行的
kernel image，寫入指定記憶體位址後跳轉過去。

```text
firmware -> bootloader.img -> UART receive kernel8.img -> jump to kernel
```

### Loading Address

如果 actual kernel 仍要載入並執行在 `0x80000`，bootloader 就不能也佔用同一段記憶體。
Basic part 可以先用 `config.txt` 要求 firmware 把 bootloader 放到其他位置：

```text
kernel_address=0x60000
kernel=bootloader.img
arm_64bit=1
```

需要搭配 linker script，讓 bootloader 的 link address 與 `kernel_address` 一致。
actual kernel 的 linker script 則可以維持從 `0x80000` 開始。

### UART Transfer Protocol

spec 只要求透過 UART 載入 binary，協定可以保持簡單。待實作時需要決定：

| 欄位 | 用途 |
| --- | --- |
| Magic | 確認 host 送來的是預期格式 |
| Image size | bootloader 要接收多少 bytes |
| Payload | raw kernel image |
| Checksum | 可選，用來檢查傳輸資料是否完整 |

最小可行版本可以先傳固定 endian 的 image size，接著連續傳送 payload。若要降低除錯成本，
建議讓 bootloader 在每個階段透過 UART 印出狀態，例如 waiting、receiving、jumping。

### Jump to Kernel

接收完成後，bootloader 需要跳到 actual kernel 的 entry address：

```c
typedef void (*kernel_entry_t)(void);

kernel_entry_t kernel = (kernel_entry_t)0x80000;
kernel();
```

實作時要注意：

- 跳轉前應確保 UART 接收已完成。
- 若有 cache 或 barrier 相關設定，需確認 image writes 對後續 instruction fetch 可見。
- 進入 kernel 前的 register contract 要和後續需求一致；devicetree 進階題會需要傳遞 `x0`。
- stack 不能放在即將被 kernel image 覆蓋的區域。

### Host Sender

Host 端可以用 Python 直接寫 serial device：

```python
with open("/dev/ttyUSB0", "wb", buffering=0) as tty:
    tty.write(...)
```

QEMU 可用 pseudo TTY 測試：

```bash
qemu-system-aarch64 -serial null -serial pty ...
```

待補：

- [ ] bootloader 與 kernel 的目錄/target 命名方式。
- [ ] host sender script 的實際 protocol。
- [ ] QEMU 測試指令。
- [ ] 實機 `config.txt` 與 SD card 放置方式。

## Initial Ramdisk

Basic exercise 2 要讓 kernel 解析 initial ramdisk。由於目前還沒有 filesystem 與 storage
driver，initramfs 會由 bootloader 或 QEMU 預先放到記憶體中，kernel 只需要從記憶體裡
解析 archive。

```text
initramfs.cpio in memory -> cpio parser -> find pathname -> file content
```

### 建立 Cpio Archive

Lab 2 使用 New ASCII Format Cpio，也就是 `newc` 格式。可先建立 `rootfs/`，放幾個純文字
檔案測試：

```bash
cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
```

QEMU 載入方式：

```bash
qemu-system-aarch64 ... -initrd initramfs.cpio
```

spec 提到 QEMU 預設會將 cpio archive 載入 `0x8000000`。

Raspberry Pi 3 可在 boot partition 放入 archive，並於 `config.txt` 指定：

```text
initramfs initramfs.cpio 0x20000000
```

### Newc Format Parser

每個 entry 由 header、pathname、file content 組成。parser 需要逐筆前進，直到遇到
`TRAILER!!!`。

待補 parser 欄位：

| 欄位 | 用途 |
| --- | --- |
| `c_magic` | 應為 `070701` |
| `c_namesize` | pathname 長度，包含結尾 `\0` |
| `c_filesize` | file content 長度 |
| pathname | entry 名稱，例如 `./hello.txt` |
| content | file data |

newc 的數值欄位是 ASCII hex；header、pathname、content 之間需要依格式做 alignment。
實作時避免把可能未對齊的位址直接 cast 成 typed pointer，先用 byte-wise parsing 較安全。

### Shell Integration

建議加上幾個 command 方便驗證：

| Command | Usage | 行為 |
| --- | --- | --- |
| `ls` | `ls` | 列出 initramfs 中的檔案 |
| `cat` | `cat <path>` | 印出指定檔案內容 |

待補：

- [ ] cpio parser API 設計。
- [ ] pathname 正規化規則，例如是否接受 `hello.txt` 和 `./hello.txt`。
- [ ] archive 結束條件與錯誤處理。
- [ ] `ls` / `cat` command 的實作細節。

## Simple Allocator

Basic exercise 3 要實作 early boot 階段使用的 simple allocator。它只需要提供連續空間，
不需要支援 `free`。

```c
void *simple_malloc(size_t size);
```

建議使用 bump allocator：

```text
heap_begin -> current bump pointer -> heap_end
```

需要決定的設計點：

| 設計點 | 說明 |
| --- | --- |
| Heap 起點 | 可由 linker symbol 或固定安全位址提供 |
| Heap 終點 | 避免覆蓋 kernel、stack、initramfs、dtb |
| Alignment | kernel data 預設以 8-byte alignment 處理 |
| OOM 行為 | 回傳 `NULL` 或直接 panic/hang |

待補：

- [ ] linker symbols 或 config 常數。
- [ ] `simple_malloc` 的 alignment 規則。
- [ ] 使用 allocator 的第一個 call site。
- [ ] OOM 的錯誤訊息與測試方式。

## Bootloader Self Relocation

Advanced exercise 1 要讓 bootloader 即使被載入到 `0x80000`，也能先搬移自己到別處，
再把 actual kernel 載入 `0x80000`。完成後就不需要依賴 `config.txt` 的
`kernel_address=`。

```text
firmware -> bootloader at 0x80000 -> copy bootloader to safe address -> continue there
                                      -> load kernel to 0x80000 -> jump
```

實作時需要特別釐清：

- bootloader image 的起訖 linker symbols。
- relocation destination 是否會和 stack、kernel、initramfs、dtb 重疊。
- copy 完成後如何跳到 relocated code 的對應位置。
- relocation 前後 absolute address、literal pool、global data 是否仍正確。

待補：

- [ ] relocation memory map。
- [ ] assembly/C 邊界設計。
- [ ] 不使用 `kernel_address=` 的實機驗證紀錄。

## Devicetree

Advanced exercise 2 要解析 flattened devicetree，也就是 dtb。目標是提供一個可遍歷
device tree 的 API，讓 driver 或 subsystem 能用 callback 檢查每個 node 與 property。
最後要用這個 API 取得 initramfs 的位址，而不是 hardcode。

```text
x0 = dtb address -> fdt parser -> traverse nodes/properties -> find initrd range
```

### Dtb Loading

QEMU 可指定 dtb：

```bash
qemu-system-aarch64 ... -dtb bcm2710-rpi-3-b-plus.dtb
```

Raspberry Pi 3 則把 dtb 放在 SD card boot partition，由 firmware 載入並把 dtb 位址放在
`x0` 傳給 kernel。若使用自己的 bootloader，bootloader 也要把這個位址傳給 actual kernel。

### FDT Parser

待補的 parser 重點：

| 區塊 | 內容 |
| --- | --- |
| Header | magic、totalsize、structure block offset、strings block offset |
| Structure block | begin node、end node、property、nop、end tokens |
| Strings block | property name 字串表 |
| Memory reservation block | 保留記憶體區域 |

遍歷 API 可以先設計成 callback 形式：

```c
void fdt_traverse(void (*callback)(...));
```

實作時要注意 dtb 使用 big-endian 欄位，不能直接用 host/native endian 解讀。

### Initramfs From Dtb

最後要從 devicetree 找出 initramfs 的範圍。待確認的 property 名稱通常會和 chosen node
有關：

| Property | 可能用途 |
| --- | --- |
| `linux,initrd-start` | initramfs 起始位址 |
| `linux,initrd-end` | initramfs 結束位址 |

待補：

- [ ] FDT header validation。
- [ ] structure block token parser。
- [ ] callback API 的參數格式。
- [ ] 讀取 `/chosen` 裡 initramfs 位址。
- [ ] bootloader 傳遞 dtb address 到 kernel 的 register contract。

## Build and Run

Lab 2 會比 Lab 1 多出幾個 artifact：

| Artifact | 用途 |
| --- | --- |
| `bootloader.img` | 由 firmware 載入，負責 UART 載入 kernel |
| `kernel8.img` | actual kernel，由 bootloader 載入並跳轉 |
| `initramfs.cpio` | initial ramdisk archive |
| `bcm2710-rpi-3-b-plus.dtb` | Raspberry Pi 3 device tree blob |

待補 Makefile target：

- [ ] build bootloader image。
- [ ] build kernel image。
- [ ] build initramfs archive。
- [ ] run QEMU with UART bootloader。
- [ ] run QEMU with `-initrd`。
- [ ] run QEMU with `-dtb`。

## 驗證紀錄

後續每完成一小段，將實際指令與結果記錄在這裡。

| 項目 | 指令 | 結果 |
| --- | --- | --- |
| Lab 2 initial build | `make` | 待補 |
| UART bootloader in QEMU | 待補 | 待補 |
| UART bootloader on Rpi3 | 待補 | 待補 |
| Initramfs list/read | 待補 | 待補 |
| Simple allocator | 待補 | 待補 |
| Devicetree traversal | 待補 | 待補 |

## 參考資料

- `Lab2/spec.pdf`
- Lab 2 online spec: `https://nycu-caslab.github.io/OSC2024/labs/lab2.html`
- FreeBSD cpio manual: `https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5`
- Devicetree specification: `https://www.devicetree.org/specifications/`
- Raspberry Pi Linux devicetree sources:
  `https://github.com/raspberrypi/linux/tree/rpi-6.6.y/arch/arm/boot/dts/broadcom`
