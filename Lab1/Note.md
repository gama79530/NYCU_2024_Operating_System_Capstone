# Lab 1 Note

Lab 1 的目標是把 Lab 0 的最小 kernel 擴充成可以透過 UART 和 host 互動的
bare-metal kernel。實作上會碰到幾個核心元件：early boot initialization、
Mini UART 與 MMIO、simple shell、mailbox，以及實機限定的 reboot。

## 實作總覽

建議把 Lab 1 拆成這幾個項目完成：

| 項目 | 主要任務 | 可能檔案 |
| --- | --- | --- |
| Boot initialization | 停住 secondary cores、清 `.bss`、設定 `sp`、進入 `main` | `boot.S`, `linker.ld` |
| Mini UART and MMIO | 提供 MMIO helpers、定義 registers、初始化 Mini UART | `util.*`, `peripheral.h`, `mini_uart.*` |
| Shell | 從 UART 讀 command，執行 `help`、`hello` 等指令 | `shell.c`, `shell.h` |
| Mailbox | 透過 property channel 查詢 board revision 和 ARM memory info | `mailbox.c`, `mailbox.h` |
| Reboot | 用 watchdog reset 實作 `reboot` command | `power.c`, `power.h` |
| Build and run | 產生 `kernel8.img`，用 QEMU 和實機驗證 | `makefile` |

建議實作順序：

1. 先完成 boot initialization，確認 kernel 能進入 `main`。
2. 完成 MMIO helpers 並初始化 Mini UART，先印出固定字串。
3. 做出 shell loop，支援 `help` 和 `hello`。
4. 實作 mailbox exchange，加入硬體資訊 command。
5. 在真實 Raspberry Pi 3 上測試 reboot。

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

## Mini UART and MMIO

Mini UART 是 kernel 與 host 的基本 I/O channel。實作時先建立 MMIO helpers，再用
它們設定 GPIO 與 AUX registers。

### MMIO Foundation

Pi 3 ARM 端以 `0x3F000000` 為 peripheral physical base；文件常見的 `0x7E...`
則是 bus address。相關職責如下：

| 檔案 | 用途 |
| --- | --- |
| `types.h` | 引入固定寬度型別、`size_t`、`bool` |
| `util.S` / `util.h` | 提供 `get32`、`put32` 與 `memzero` |
| `util.c` | 以 loop 和 `nop` 實作 `wait_cycles` |
| `peripheral.h` | 定義 GPIO 與 AUX register addresses |

```c
uint32_t get32(uintptr_t addr);
void put32(uintptr_t addr, uint32_t value);
```

MMIO registers 使用 32-bit access；`get32`、`put32` 分別由 `ldr w0`、`str w1`
實作。

### GPIO and UART Initialization

GPIO 14/15 分別作為 TXD1/RXD1，必須在 `GPFSEL1` 設為 ALT5，再依序設定
pull control 與 Mini UART：

```c
selector = get32(GPFSEL1);
selector &= ~((7 << 12) | (7 << 15));
selector |= (2 << 12) | (2 << 15);
put32(GPFSEL1, selector);

put32(GPPUD, 0);
wait_cycles(150);
put32(GPPUDCLK0, (1 << 14) | (1 << 15));
wait_cycles(150);
put32(GPPUDCLK0, 0);

put32(AUX_ENABLES, 1);
put32(AUX_MU_CNTL, 0);
put32(AUX_MU_LCR, 3);
put32(AUX_MU_MCR, 0);
put32(AUX_MU_IER, 0);
put32(AUX_MU_IIR, 0xc6);
put32(AUX_MU_BAUD, 270);
put32(AUX_MU_CNTL, 3);
```

Lab 1 使用 polling，因此關閉 interrupts；寫入 `AUX_MU_IIR = 0xc6` 會清除 RX/TX
FIFOs。Baud register 設為 `270`，搭配 250 MHz core clock 得到約 115200 baud。

### Polling I/O

- `mini_uart_getb` 等待 `AUX_MU_LSR[0]`，再從 `AUX_MU_IO` 讀取 byte。
- `mini_uart_putb` 等待 `AUX_MU_LSR[5]`，再將 byte 寫入 `AUX_MU_IO`。
- `mini_uart_getc` 將輸入的 `\r` 轉為 `\n`。
- `mini_uart_putc` 將輸出的 `\n` 轉為 `\r\n`。
- `mini_uart_puts`、`mini_uart_putln` 建立在 `mini_uart_putc` 之上。

## Simple Shell

Shell 透過 UART 讀取一行文字，解析成 `argc/argv`，再從 command table 找到對應
handler。輸出統一使用 tiny `printf`，其 callback 最後呼叫 `mini_uart_putc`。主迴圈
每次顯示 `$ ` prompt，執行完成後繼續等待下一行。

Kernel 進入 shell 前依序初始化 UART 與 formatted output：

```c
mini_uart_init();
init_printf(NULL, printf_putc);
shell_run();
```

`printf_putc(void *context, char c)` 是 callback adapter，用來配合 tiny `printf` 的
函式型別；實際字元輸出與 `\n` 到 `\r\n` 的轉換仍由 `mini_uart_putc` 負責。

Command table 保存名稱、usage、description 與 handler：

| Command | Usage | 行為 |
| --- | --- | --- |
| `help` | `help [command]` | 列出 commands，或顯示指定 command 的說明 |
| `hello` | `hello` | 印出 `Hello World!` |
| `mailbox` | `mailbox [revision\|memory]...` | 顯示全部或指定的硬體資訊 |
| `reboot` | `reboot` | 透過 watchdog 重新啟動 Raspberry Pi |

新增 command 時，只需加入 handler 與一筆 table entry。`shell_find_command` 同時供
dispatcher 和 `help` 使用，usage 也統一由 table 取得。

輸入與解析規則：

- Echo 可見 ASCII；Enter 完成一行。
- `\b` 和 `0x7f` 都視為刪除，輸出 `\b \b` 更新畫面。
- 分別記錄 buffer length 與 display length；輸入超長後仍可 Backspace 回合法長度。
- Parser 忽略開頭、結尾與連續空白，並直接在 buffer 中以 `\0` 分隔 arguments。
- 空行不執行 command；未知 command、過長輸入與過多 arguments 會顯示錯誤。

Shell 使用固定大小的 buffer 和 `argv` array，容量由 `config.h` 的
`CONFIG_SHELL_BUFFER_SIZE`、`CONFIG_SHELL_MAX_ARGS` 控制，不需要 heap。

## Mailbox Property Interface

Raspberry Pi 3 有些硬體資訊與 peripheral 設定由 VideoCore firmware 管理。ARM CPU
透過 property channel 傳遞 request buffer。本 Lab 使用它查詢 board revision 與
ARM memory base/size，並由 `mailbox` shell command 顯示結果。

```text
mailbox                    # 顯示全部資訊
mailbox revision           # 只顯示 board revision
mailbox memory             # 只顯示 ARM memory
mailbox revision memory    # 顯示兩者
```

參數會先全部驗證，再執行查詢；未知參數不會產生部分輸出，重複參數也只查詢一次。

### Message Format

Message 是 16-byte aligned 的 `volatile uint32_t` array；encoded request 的高 28 bits
放 buffer address，低 4 bits 放 channel `8`。整體格式如下：

| Index | 內容 |
| --- | --- |
| `buffer[0]` | 整個 message buffer 大小，單位 byte |
| `buffer[1]` | request / response code |
| `buffer[2...]` | tag ID、value buffer size、tag status 與 values |
| 最後一個 word | end tag `0` |

Firmware 會把 tag status 的 bit 31 設為 response bit，低 31 bits 則表示實際回傳
長度。例如 `0x80000004` 代表成功回傳 4 bytes。`mailbox_validate_tag` 因此檢查：

- response bit 已設定。
- 回傳長度不少於呼叫端需要的資料量。
- 回傳長度沒有超過 tag 的 value buffer 容量。

### Exchange Flow

1. 填好 message，等待 mailbox 可寫。
2. 執行 `dmb sy`，確保 message writes 排在 `MAILBOX_WRITE` 之前，再送出 request。
3. 等待 mailbox 可讀，忽略不屬於本次 request 的 response。
4. 收到相符 response 後執行 `dmb sy`，再讀取 firmware 更新的 buffer。
5. 驗證整體 response code 與 tag status。

`dmb sy` 是記憶體存取的順序邊界，不是關閉 reordering 的開關；它只要求邊界前後的
memory accesses 維持順序，也不會清除或刷新 cache。

所有 polling 都受 `CONFIG_MAILBOX_TIMEOUT` 限制。這個值是 polling 次數上限，不是
固定時間；用途是避免 firmware 或位址設定異常時讓 kernel 永久卡住。底層錯誤以
`mailbox_error_t` 回傳，Shell 再透過 `mailbox_error_string` 顯示原因。

`mailbox_get_board_revision` 使用 tag `0x00010002`；`mailbox_get_arm_memory` 使用
tag `0x00010005`。兩者共用一個 aligned buffer，符合目前 single-core、循序執行
command 的設計。

### Verbose Output

`config.h` 的 `CONFIG_VERBOSE` 控制額外執行資訊，目前設為 `0`。開啟時設為 `1`，
設為 `0` 時 `LOG_VERBOSE` 會在編譯期移除。Verbose output 包含 Mailbox request/response 與
reboot ticks；一般輸出與錯誤訊息不受影響。Log 不放在 polling loop 內，避免
timeout 時大量輸出。

## Reboot Command

`reboot` 是進階功能，做法是透過 power management watchdog 觸發 full reset。

```c
#define PM_PASSWORD        0x5A000000
#define PM_RSTC_FULL_RESET 0x00000020

void power_reboot(void)
{
    put32(PM_RSTC, PM_PASSWORD | PM_RSTC_FULL_RESET);
    data_memory_barrier();
    put32(PM_WDOG, PM_PASSWORD | CONFIG_REBOOT_TICKS);
    data_sync_barrier();

    while (true) {
        asm volatile("wfe");
    }
}
```

`PM_PASSWORD` 是寫入 power management registers 時必須附加的 password；
`PM_RSTC_FULL_RESET` 選擇完整重啟，watchdog ticks 決定多久後觸發。ticks 由
`CONFIG_REBOOT_TICKS` 統一設定，並限制在 watchdog 的 20-bit time field 內。

| Barrier | 全名 | 用途 |
| --- | --- | --- |
| `dmb sy` | Data Memory Barrier | 保證 barrier 前後的 memory accesses 維持順序 |
| `dsb sy` | Data Synchronization Barrier | 等待 barrier 前的 memory accesses 完成 |

此處的 `dmb sy` 保證 reset mode 先於 watchdog 生效；`dsb sy` 則確認兩次 MMIO
writes 都已完成，再進入 `wfe`。`sy` 表示 full-system scope。

`power_reboot` 標記為 `noreturn`，因為成功時系統會重啟；若平台沒有實作 reset，
CPU 也只會停在等待迴圈，不會錯誤地返回 Shell。

此功能只保證在真實 Raspberry Pi 3 上運作。QEMU 可以驗證 command 與程式流程，
但不保證會真的 reboot。

## 參考資料

- `Lab1/spec.pdf`
- `/home/rein/workspace/NYCU_2024_Operating_System_Capstone-bk2/Lab1/note.md`
- `/home/rein/workspace/NYCU_2024_Operating_System_Capstone_bk/Lab1/recaps.md`
- BCM2837 ARM Peripherals: `https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf`
- Raspberry Pi firmware mailbox property interface:
  `https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface`
