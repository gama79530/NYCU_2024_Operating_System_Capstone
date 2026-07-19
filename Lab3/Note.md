# Lab 3 Note

Lab 3 的目標是把 Lab 2 的 booting、initramfs 與 shell 基礎延伸到
exception 和 interrupt。核心要先從 EL2 切到 EL1 執行 kernel，再能透過
`eret` 進入 EL0 user program；當 user program 透過 `svc` 或 timer 觸發 exception
時，CPU 會回到 EL1 的 exception vector，由 kernel 保存 context、判斷原因並處理。

這次也會把前面使用 busy polling 的 I/O 逐步改成 interrupt-driven 的形式，最後在
advanced exercises 中用 one-shot core timer multiplex 多個 software timer，並加入
task queue 來延後處理較重的 interrupt work。

## 實作總覽

建議把 Lab 3 拆成這幾個項目完成：

| 項目 | 主要任務 | 可能檔案 |
| --- | --- | --- |
| EL2 to EL1 | kernel early boot 時設定 `HCR_EL2`、`SPSR_EL2`、`ELR_EL2`，用 `eret` 進入 EL1h | `boot.S`, `util.S` |
| EL1 to EL0 | 新增 shell command 從 initramfs 載入 user program，設定 `SPSR_EL1`、`ELR_EL1`、`SP_EL0` 後 `eret` | `shell.c`, `exception.*`, `initramfs.*` |
| Exception vector | 建立 0x800-aligned vector table，設定 `VBAR_EL1`，處理 sync/IRQ entries | `exception.S`, `exception.c`, `linker.ld` |
| Context saving | exception entry 保存/還原 general purpose registers，必要時保存 `SPSR_EL1`、`ELR_EL1` | `exception.S`, `trap_frame.h` |
| Core timer interrupt | 啟用 physical timer、unmask core0 timer IRQ，handler 印 boot seconds 並重設下一次 timeout | `timer.*`, `interrupt.*`, `peripheral.h` |
| Mini UART interrupt | 啟用 AUX interrupt 與 IRQ1 bit29，用 RX/TX buffer 做非同步 UART I/O | `mini_uart.*`, `interrupt.*`, `ring_buffer.*` |
| Timer multiplexing | 以 one-shot timer 管理 timer queue，實作 `setTimeout MESSAGE SECONDS` | `timer.*`, `shell.c` |
| Task queue | interrupt handler 只排 task，回 user 前開 interrupt 執行 task，支援 priority/preemption | `task_queue.*`, `interrupt.*` |

建議實作順序：

1. 先從 Lab 2 複製 kernel，確認 `Lab3/c` 可 build 並保留 initramfs shell。
2. 在最早的 boot path 加入 EL2 to EL1，確認 `CurrentEL` 已進入 EL1。
3. 建立 exception vector table，先讓所有 entry 進同一個 handler 並印 `SPSR_EL1`、`ELR_EL1`、`ESR_EL1`。
4. 實作 context save/restore，確保 user program 多次 `svc` 後暫存器不被 kernel 破壞。
5. 新增從 initramfs 載入 user program 的 shell command，設定 `SP_EL0` 後 `eret` 進 EL0。
6. 啟用 core timer IRQ，先只在 EL0 開 interrupt，handler 每 2 秒印一次 boot time。
7. 再做 mini UART interrupt 與 async buffer。
8. 最後處理 advanced 的 timer queue、task queue、nested interrupt 與 priority preemption。

## Exception Levels

Raspberry Pi 3 firmware/QEMU 預設會讓 kernel 從 EL2 開始跑，但本 lab 希望 kernel
位於 EL1，user program 位於 EL0。整體控制流如下：

```text
firmware/QEMU -> kernel _start at EL2
              -> configure EL1 state
              -> eret to EL1 kernel main
              -> shell command loads user program
              -> eret to EL0 user code
              -> svc/timer/irq
              -> EL1 exception handler
              -> eret back to EL0
```

### EL2 to EL1

切換到 EL1 時，至少要設定：

| Register | 用途 |
| --- | --- |
| `HCR_EL2` | 設定 bit 31，讓 EL1 使用 AArch64 |
| `SPSR_EL2` | 設定 return 後的 PSTATE，例如 EL1h 且 interrupt disabled |
| `ELR_EL2` | 放入 `eret` 後要執行的 EL1 target address |

spec 範例使用 `SPSR_EL2 = 0x3c5`，代表回到 EL1h、使用 `SP_EL1`，並先關閉
debug、SError、IRQ、FIQ。建議在 `_start` 停住 secondary cores 後立即完成這件事，
讓後續 C code 都在 EL1 的假設下執行。

### EL1 to EL0

進入 user program 前，kernel 需要準備：

| Register | 建議內容 |
| --- | --- |
| `SPSR_EL1` | basic timer 前可先用 `0x3c0`；若要讓 EL0 接收 IRQ，改成不 mask IRQ 的狀態 |
| `ELR_EL1` | user program entry address |
| `SP_EL0` | user stack top，需維持 16-byte alignment |

user program 可以先放在 initramfs，透過 shell command 讀入指定檔案內容並跳轉。因為
initramfs 檔案內容不一定天然對齊，載入 user program 時要注意 instruction/data alignment，
必要時用 allocator 配置 aligned buffer 再複製。

## Exception Vector Table

CPU 發生 exception 後，會根據 `VBAR_EL1` 指向的 vector table 跳到對應 entry。vector
table base 必須 0x800 aligned，每個 entry 大小是 0x80 bytes。

本 lab 主要會用到：

| Entry 類型 | 情境 |
| --- | --- |
| Current EL with SP_ELx, Sync | kernel 在 EL1 內部發生 synchronous exception |
| Current EL with SP_ELx, IRQ | kernel 開啟 EL1 interrupt 後收到 IRQ |
| Lower EL AArch64, Sync | EL0 user program 執行 `svc` |
| Lower EL AArch64, IRQ | EL0 user program 被 timer/UART interrupt 打斷 |

初期可以讓所有 entries 都 branch 到同一個 `exception_handler`，先印出：

| Register | 意義 |
| --- | --- |
| `SPSR_EL1` | exception 發生前的 PSTATE |
| `ELR_EL1` | exception return address |
| `ESR_EL1` | synchronous exception 或 SError 的原因 |

後續再依 entry 或 `ESR_EL1.EC` 區分 syscall、data abort、unknown exception 等情況。

## Context Saving

exception handler 和 user program 共用 general purpose registers，因此在進 C handler
前必須先保存暫存器，回去前再還原：

```asm
exception_handler:
    save_all
    bl exception_entry
    load_all
    eret
```

basic 的 `save_all` 可以保存 `x0` 到 `x30`。advanced 做 nested interrupt/task queue 時，
還需要一起保存 `SPSR_EL1` 與 `ELR_EL1`，否則內層 interrupt 可能覆蓋外層 exception
return state。

建議把 stack frame layout 固定成 C 也能讀的 `trap_frame` 結構，避免 assembly 和 C
各自用 magic offset。

## Core Timer Interrupt

每個 core 都有自己的 physical core timer。啟用流程：

| 步驟 | 操作 |
| --- | --- |
| 啟用 timer | `cntp_ctl_el0 = 1` |
| 設定到期時間 | 寫 `cntp_tval_el0`，例如 `2 * cntfrq_el0` |
| unmask core0 timer IRQ | `CORE0_TIMER_IRQ_CTRL = 0x40000040`，寫入 bit 1 |
| 開 CPU IRQ | basic 只在 EL0 開；advanced 也可在 EL1 用 `DAIFClr` 開 |

handler 中可用 `cntpct_el0 / cntfrq_el0` 算 boot 後秒數，印出時間後再設定下一次
timeout。basic exercise 要求下一次 timeout 是 2 秒後。

## Mini UART Interrupt

前面 Lab 的 shell 使用 polling：

```text
shell -> mini_uart_getc/putc -> busy wait AUX_MU_LSR
```

Lab 3 需要加入 interrupt-driven path：

```text
RX IRQ -> read AUX_MU_IO -> push read buffer
shell -> pop read buffer

shell/printf -> push write buffer
TX IRQ -> pop write buffer -> write AUX_MU_IO
```

需要啟用兩層 interrupt controller：

| Controller | Register | 設定 |
| --- | --- | --- |
| Mini UART | `AUX_MU_IER_REG` / `AUX_MU_IER` | enable RX/TX interrupt |
| BCM peripheral IRQ | `ENABLE_IRQS1` at `0x3f00b210` | set bit 29 for AUX interrupt |

收到 IRQ 後不能假設只有 UART 或只有 timer。interrupt dispatch 要先檢查 core local
interrupt controller，再查 BCM peripheral interrupt pending registers，確認來源後才呼叫對應
handler。

## Timer Multiplexing

core timer 是 one-shot timer，一次只能設定一個到期點。advanced exercise 要做 software
timer queue：

```text
add_timer(callback, data, after_ticks)
    -> calculate expires_at
    -> insert into sorted timer queue
    -> if new timer is earliest, reprogram core timer

timer IRQ
    -> pop and run expired callbacks
    -> program next earliest timer
```

`setTimeout MESSAGE SECONDS` 是測試用 shell command，必須 non-blocking。使用者可以
連續設定多個 timeout，實際印出順序由 command 執行時間加上指定秒數決定。

## Task Queue and Nested Interrupt

advanced 的 concurrent I/O handling 目標是縮短 interrupt disabled 的時間。interrupt handler
只做必要工作：

1. mask 該 device interrupt。
2. 從 device buffer 搬走資料，或記錄需要處理的狀態。
3. enqueue processing task。
4. 回 user 前，在 interrupt context 中開啟 interrupt 執行 queued tasks。
5. task 完成後 unmask device interrupt。

為了支援 nested interrupt，exception frame 必須保存 general registers、`SPSR_EL1`、
`ELR_EL1`。若加入 priority preemption，回到前一層 handler 前要檢查 queue 中是否有更高
priority task，若有則先執行最高 priority task。

## Build and Run

Lab 3 先沿用 Lab 2 的 build flow：

```bash
make -C Lab3/c
make -C Lab3/c qemu
```

若使用共用 UART bootloader 上傳 actual kernel：

```bash
make lab3 SERIAL_PORT=/dev/ttyUSB0
python3 KernelUploader.py --kernel ./Lab3/c/bin/kernel8.img --port /dev/ttyUSB0
```

目前 Lab 3 的 kernel code 是從 Lab 2 複製而來；開始實作 exception/interrupt 前，先確認
`Lab3/c/include/config.h` 與各 header guard 已改成 Lab 3 名稱，避免後續 include macro
誤導閱讀。

## References

- `Lab3/spec.pdf`
- Lab 3 online spec: `https://nycu-caslab.github.io/OSC2024/labs/lab3.html`
- ARMv8-A exception handling and vector table
- BCM2837 ARM Peripherals interrupt controller
- Raspberry Pi 3 core local interrupt controller
