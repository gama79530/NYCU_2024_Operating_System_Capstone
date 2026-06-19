# Lab 1 Note

Lab 1 的目標是把 Lab 0 的最小 kernel 擴充成可以透過 UART 和 host 互動的
bare-metal kernel。實作上會碰到幾個核心元件：early boot initialization、
MMIO helper、mini UART、simple shell、mailbox，以及實機限定的 reboot。

## 實作總覽

建議把 Lab 1 拆成這幾個項目完成：

| 項目 | 主要任務 | 可能檔案 |
| --- | --- | --- |
| Boot initialization | 停住 secondary cores、清 `.bss`、設定 `sp`、進入 `main` | `boot.S`, `linker.ld` |
| MMIO helpers | 提供 `get32`、`put32`、`wait_cycles` 等低階操作 | `util.S`, `util.c`, `util.h` |
| Peripheral definitions | 集中定義 GPIO、AUX、mailbox、power registers | `peripheral.h` |
| Mini UART | 初始化 GPIO 14/15 和 mini UART，提供 polling I/O | `mini_uart.c`, `mini_uart.h` |
| Shell | 從 UART 讀 command，執行 `help`、`hello` 等指令 | `shell.c`, `shell.h` |
| Mailbox | 透過 property channel 查詢 board revision 和 ARM memory info | `mailbox.c`, `mailbox.h` |
| Reboot | 用 watchdog reset 實作 `reboot` command | `power.c`, `power.h` |
| Build and run | 產生 `kernel8.img`，用 QEMU 和實機驗證 | `makefile` |

建議實作順序：

1. 先完成 boot initialization，確認 kernel 能進入 `main`。
2. 補上 MMIO helper 和 peripheral register definitions。
3. 初始化 mini UART，先印出固定字串。
4. 做出 shell loop，支援 `help` 和 `hello`。
5. 實作 mailbox exchange，加入硬體資訊 command。
6. 在真實 Raspberry Pi 3 上測試 reboot。

## Boot Initialization

Firmware 將 `kernel8.img` 載入 `0x80000` 後，kernel 必須自行完成最基本的
執行環境：停住 secondary cores、設定 stack、清除 `.bss`，最後進入 C code。

```text
firmware -> _start -> core check -> stack -> clear BSS -> main -> hang
```

### `linker.ld`

Linker script 決定 kernel 的記憶體配置，並提供 boot code 需要的 symbols。

| 設定 | 用途 |
| --- | --- |
| `ENTRY(_start)` | 指定 ELF entry point |
| `kernel_begin = 0x80000` | 配合 firmware 的 image load address |
| `.text.boot` | 將 `_start` 放在 kernel image 最前面 |
| `.text/.rodata/.data/.bss` | 依用途排列程式與資料 |
| `bss_begin`、`bss_size` | 提供 `boot.S` 清除 BSS 所需的範圍 |
| `ALIGN(0x10)` | 維持 16-byte section alignment |

`PHDRS` 將 sections 分成 `R-X`、`R--`、`RW-` segments，主要描述 ELF layout；
轉成 raw `kernel8.img` 後不會保留這些 headers。

### `boot.S`

`_start` 是 firmware 載入 image 後最先執行的位置：

```asm
_start:
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    cbnz    x0, hang

    ldr     x0, =_start
    mov     sp, x0

    ldr     x0, =bss_begin
    ldr     x1, =bss_size
    bl      memzero

    bl      main

hang:
    wfe
    b       hang
```

- `MPIDR_EL1` 的 Aff0 表示 core id，只有 core 0 繼續啟動。
- `sp = 0x80000` 且 stack 向低位址成長，不會立即覆蓋向高位址排列的 kernel。
- `memzero(bss_begin, bss_size)` 必須在進入 C code 前完成。
- `sp` 在呼叫函式前必須有效並維持 16-byte alignment。
- `.type`、`.size` 只補充 ELF symbol metadata，不會產生指令。

### `main.c`

目前還沒有需要初始化的 kernel service，因此 `main` 可以先保持空白：

```c
void main(void)
{
}
```

`main` 返回後會進入 `boot.S` 的 `hang`。完成 Mini UART 後，再從這裡呼叫 UART
initialization 和 shell。

### 需要互相搭配的重點

| 搭配項目 | 必須一致的原因 |
| --- | --- |
| Firmware 與 linker | load address 和 `kernel_begin` 都是 `0x80000` |
| Linker 與 `boot.S` | linker 提供 `_start`, `bss_begin`, `bss_size` |
| `boot.S` 與 `util.S` | `memzero` 接收 `x0` 的起點與 `x1` 的大小 |
| `boot.S` 與 `main.c` | stack, BSS 準備完成後才能呼叫 `main` |

建置後可確認 entry point 與 symbols：

```bash
aarch64-linux-gnu-readelf -h -s build/kernel8.elf
```

## MMIO Helpers and Peripheral Addresses

Raspberry Pi 3 透過 memory-mapped I/O 存取 peripheral registers。也就是說，
程式讀寫某些 physical address 時，實際上是在讀寫硬體 register，而不是一般 RAM。

Raspberry Pi peripheral 文件常用 bus address 描述 register，例如 `0x7E...`。
ARM CPU 實際存取的是 physical address。Raspberry Pi 3 的 peripheral bus base
`0x7E000000` 對應到 physical MMIO base `0x3F000000`。

常用 base：

```c
#define MMIO_BASE 0x3F000000
#define GPIO_BASE (MMIO_BASE + 0x00200000)
#define AUX_BASE  (MMIO_BASE + 0x00215000)
#define MBOX_BASE (MMIO_BASE + 0x0000B880)
```

MMIO access 要使用 `volatile`，避免 compiler 把 register access 最佳化掉：

```c
static inline void put32(unsigned long addr, unsigned int value)
{
    *(volatile unsigned int *) addr = value;
}

static inline unsigned int get32(unsigned long addr)
{
    return *(volatile unsigned int *) addr;
}
```

Lab 1 還會需要 delay helper，例如 GPIO pull-up/down 設定流程中的 `wait_cycles`。
這類 helper 可以用 assembly 做一個簡單 busy wait。

## Mini UART

Lab 1 使用 mini UART，而不是 PL011 UART。之後所有 lab 都會把 UART 當成 host 和
Raspberry Pi 之間的基本 I/O channel。

### GPIO 14/15

mini UART 使用 GPIO 14/15：

| GPIO | 功能 |
| --- | --- |
| GPIO 14 | TXD1 |
| GPIO 15 | RXD1 |

這兩個 pin 要在 `GPFSEL1` 設成 ALT5：

```c
selector = get32(GPFSEL1);
selector &= ~((7 << 12) | (7 << 15));
selector |=  (2 << 12) | (2 << 15);
put32(GPFSEL1, selector);
```

接著關閉 GPIO pull-up/down：

```c
put32(GPPUD, 0);
wait_cycles(150);
put32(GPPUDCLK0, (1 << 14) | (1 << 15));
wait_cycles(150);
put32(GPPUDCLK0, 0);
```

### Mini UART Registers

| Register | 位址 |
| --- | --- |
| `AUX_ENABLES` | `MMIO_BASE + 0x00215004` |
| `AUX_MU_IO` | `MMIO_BASE + 0x00215040` |
| `AUX_MU_IER` | `MMIO_BASE + 0x00215044` |
| `AUX_MU_IIR` | `MMIO_BASE + 0x00215048` |
| `AUX_MU_LCR` | `MMIO_BASE + 0x0021504C` |
| `AUX_MU_MCR` | `MMIO_BASE + 0x00215050` |
| `AUX_MU_LSR` | `MMIO_BASE + 0x00215054` |
| `AUX_MU_CNTL` | `MMIO_BASE + 0x00215060` |
| `AUX_MU_BAUD` | `MMIO_BASE + 0x00215068` |

### Initialization Sequence

```c
put32(AUX_ENABLES, 1);
put32(AUX_MU_CNTL, 0);
put32(AUX_MU_LCR, 3);
put32(AUX_MU_MCR, 0);
put32(AUX_MU_IER, 0);
put32(AUX_MU_IIR, 0xc6);
put32(AUX_MU_BAUD, 270);
put32(AUX_MU_CNTL, 3);
```

說明：

- `AUX_ENABLES = 1`：啟用 mini UART register access。
- `AUX_MU_CNTL = 0`：設定期間先關閉 transmitter / receiver。
- `AUX_MU_LCR = 3`：設定 8-bit mode。
- `AUX_MU_IER = 0`：Lab 1 使用 polling，不啟用 UART interrupt。
- `AUX_MU_IIR = 0xc6`：清 FIFO / 關閉 interrupt 相關狀態。
- `AUX_MU_BAUD = 270`：設定 115200 baud。
- `AUX_MU_CNTL = 3`：最後啟用 transmitter 和 receiver。

### Polling I/O

讀一個 byte：

```c
while (!(get32(AUX_MU_LSR) & 0x01)) {
}
c = get32(AUX_MU_IO) & 0xff;
```

寫一個 byte：

```c
while (!(get32(AUX_MU_LSR) & 0x20)) {
}
put32(AUX_MU_IO, c);
```

Serial console 常見換行問題：

- 輸入端可能送 `\r`，kernel 可以轉成 `\n`。
- 輸出端印 `\n` 前先補 `\r`，也就是輸出 `\r\n`，避免畫面只換行不回到行首。

## Simple Shell

Shell 的工作是用 UART 讀入一行 command，解析後呼叫對應 handler。

Lab 1 spec 要求的基本 command：

| Command | 行為 |
| --- | --- |
| `help` | 印出所有支援的 commands |
| `hello` | 印出 `Hello World!` |

後續為了 mailbox 和 reboot，可以一起規劃：

| Command | 行為 |
| --- | --- |
| `mailbox` | 查詢或列出 mailbox 相關資訊 |
| `reboot` | 觸發 watchdog reset，實機限定 |

Shell loop：

1. 印 prompt，例如 `# `。
2. 從 UART 逐字讀入。
3. Echo 可見字元。
4. 遇到 Enter 時結束一行。
5. 解析 command。
6. 執行 handler 後回到 prompt。

輸入處理要注意：

- `\r` / `\n`：視為 Enter。
- Backspace：刪除 buffer 最後一個字元，畫面輸出 `\b \b`。
- Buffer 長度：避免 command buffer overflow。
- 空行：直接重新印 prompt。
- 不支援的 command：印出錯誤訊息。

Command dispatch 可以先用 `if/else`，Lab 1 command 數量還很少；之後 command 變多
再改成 table 也可以。

## Mailbox Property Interface

Raspberry Pi 3 有些硬體資訊與 peripheral 設定由 VideoCore firmware 管理。ARM CPU
透過 mailbox 傳遞 request buffer，讓 firmware 執行查詢或設定。

Lab 1 至少要查詢並印出：

- Board revision。
- ARM memory base address。
- ARM memory size。

### Registers and Channel

```c
#define MBOX_BASE   (MMIO_BASE + 0x0000B880)
#define MBOX_READ   (MBOX_BASE + 0x00)
#define MBOX_STATUS (MBOX_BASE + 0x18)
#define MBOX_WRITE  (MBOX_BASE + 0x20)

#define MBOX_EMPTY 0x40000000
#define MBOX_FULL  0x80000000

#define MBOX_CH_PROP 8
```

### Message Buffer

Mailbox property buffer 是 32-bit word array。因為 message 的低 4 bits 會拿來放
channel number，所以 buffer 必須 16-byte aligned。

```c
volatile unsigned int __attribute__((aligned(16))) mailbox[36];
```

Buffer 格式：

| Index | 內容 |
| --- | --- |
| `buffer[0]` | 整個 message buffer 大小，單位 byte |
| `buffer[1]` | request / response code |
| `buffer[2...]` | 一個或多個 tags |
| 最後 | end tag，值為 `0` |

常用 code：

```c
#define MBOX_REQUEST     0x00000000
#define MBOX_RESPONSE    0x80000000
#define MBOX_TAG_REQUEST 0x00000000
#define MBOX_TAG_END     0x00000000
```

Tag 格式：

| 欄位 | 意義 |
| --- | --- |
| tag identifier | 要查詢或設定的項目 |
| value buffer size | value buffer 長度，單位 byte |
| request / response code | request 時填 `0` |
| value buffer | request 參數或 response 結果 |

### Exchange Flow

1. 把 request 寫入 16-byte aligned buffer。
2. 把 buffer address 的低 4 bits 清掉，再 OR 上 channel number。
3. 等 `MBOX_STATUS` 沒有 `MBOX_FULL`。
4. 寫入 `MBOX_WRITE`。
5. 等 `MBOX_STATUS` 沒有 `MBOX_EMPTY`。
6. 讀 `MBOX_READ`，確認回來的值等於送出的 encoded address。
7. 確認 `buffer[1] == MBOX_RESPONSE`。

Pseudo code：

```c
unsigned int msg = ((unsigned long) buffer & ~0xf) | (channel & 0xf);

while (get32(MBOX_STATUS) & MBOX_FULL) {
}
put32(MBOX_WRITE, msg);

do {
    while (get32(MBOX_STATUS) & MBOX_EMPTY) {
    }
} while (get32(MBOX_READ) != msg);

return buffer[1] == MBOX_RESPONSE;
```

### Query Board Revision

```c
#define MBOX_TAG_GETREVISION 0x00010002

mailbox[0] = 7 * 4;
mailbox[1] = MBOX_REQUEST;
mailbox[2] = MBOX_TAG_GETREVISION;
mailbox[3] = 4;
mailbox[4] = MBOX_TAG_REQUEST;
mailbox[5] = 0;
mailbox[6] = MBOX_TAG_END;
```

成功後：

```c
revision = mailbox[5];
```

### Query ARM Memory

```c
#define MBOX_TAG_GETMEMORY 0x00010005

mailbox[0] = 8 * 4;
mailbox[1] = MBOX_REQUEST;
mailbox[2] = MBOX_TAG_GETMEMORY;
mailbox[3] = 8;
mailbox[4] = MBOX_TAG_REQUEST;
mailbox[5] = 0;
mailbox[6] = 0;
mailbox[7] = MBOX_TAG_END;
```

成功後：

```c
arm_memory_base = mailbox[5];
arm_memory_size = mailbox[6];
```

## Reboot Command

`reboot` 是進階功能，做法是透過 power management watchdog 觸發 full reset。

```c
#define PM_PASSWORD 0x5a000000
#define PM_RSTC     0x3F10001c
#define PM_WDOG     0x3F100024

void reset(int tick)
{
    put32(PM_RSTC, PM_PASSWORD | 0x20);
    put32(PM_WDOG, PM_PASSWORD | tick);
}
```

注意：

- 這段只保證在真實 Raspberry Pi 3 上可用。
- QEMU 上可能不會真的 reboot。
- `tick` 不要設太大，否則會看起來像沒有反應。

## Suggested File Layout

後續實作 Lab 1 時，可以先用類似下面的結構：

```text
Lab1/
  include/
    mailbox.h
    mini_uart.h
    peripheral.h
    power.h
    shell.h
    string.h
    util.h
  src/
    boot.S
    linker.ld
    mailbox.c
    mini_uart.c
    main.c
    power.c
    shell.c
    string.c
    util.S
    util.c
  makefile
  spec.pdf
  Note.md
```

## Build and Run

QEMU 執行：

```bash
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio
```

如果 Makefile 已經包好：

```bash
make
make run
```

實機執行時，把 `kernel8.img` 放到 SD card boot partition，然後用 serial console：

```bash
sudo screen /dev/ttyUSB0 115200
```

## Verification Checklist

- Boot code 只讓 core 0 進入 kernel。
- `.bss` 會在進入 C code 前清成 0。
- `sp` 在進入 C code 前已設定。
- MMIO base 使用 Raspberry Pi 3 的 `0x3F000000`。
- MMIO access 使用 `volatile`。
- GPIO 14/15 已切到 ALT5。
- mini UART 使用 115200 baud、8-bit mode、polling I/O。
- 輸出 `\n` 時會補 `\r`。
- 開機後 UART 有印 prompt。
- `help` 能列出 commands。
- `hello` 印出 `Hello World!`。
- Enter 換行正常，畫面不會越跑越右邊。
- Backspace 不會讓 command buffer underflow。
- Mailbox buffer 是 16-byte aligned。
- Mailbox 使用 property channel 8。
- Mailbox 至少印出 board revision、ARM memory base、ARM memory size。
- `reboot` 在真實 Raspberry Pi 3 上會重開機。

## 參考資料

- `Lab1/spec.pdf`
- `/home/rein/workspace/NYCU_2024_Operating_System_Capstone-bk2/Lab1/note.md`
- `/home/rein/workspace/NYCU_2024_Operating_System_Capstone_bk/Lab1/recaps.md`
- BCM2837 ARM Peripherals: `https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf`
- Raspberry Pi firmware mailbox property interface:
  `https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface`
