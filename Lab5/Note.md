# Lab 5 Note

Lab 5 要在既有 allocator 與 exception handling 的基礎上建立多工機制，讓 kernel
能建立 threads、切換執行 context，並讓 EL0 user processes 透過 system calls
使用 kernel 服務。依照 spec 整理成六個相互銜接的實作目標：

1. Thread and Scheduler
2. Kernel Preemption
3. User Process and Basic System Calls
4. Process Operations
5. Video Player Integration
6. POSIX Signal

初始化與執行流程建議如下：

```text
初始化 allocator 與 exception handling
    -> 初始化 thread system 與 idle thread
    -> 建立 kernel threads，驗證 context switch
    -> 啟用 timer，驗證 kernel preemption
    -> 建立 user process、基本 syscalls 與 user preemption
    -> 完成 exec、fork、kill 與 mailbox
    -> 執行 Video Player
    -> 加入 signal delivery 與 context restore
```

以下 Goal 依實作順序排列，每個階段先驗證可執行的結果，再加入下一層功能。

## Goal 1: Thread and Scheduler

實作具有獨立 stack 與 register context 的 threads，並提供 `schedule()`，讓相同
priority 的 runnable threads 以 round-robin 方式輪流執行。這裡的 thread 需要
能保存與恢復自己的執行位置，不能只沿用共用 stack 的 deferred task queue。

建議資料結構：

| 資料結構 | 用途 |
| --- | --- |
| thread metadata | 保存 thread ID、state、stack 範圍與 queue node |
| CPU context | 保存 context switch 所需的 callee-saved registers 與 SP |
| run queue | 保存可被 scheduler 選取的 runnable threads |
| zombie list | 保存已結束、等待回收 stack 與 metadata 的 threads |
| idle thread | 沒有其他 runnable thread 時執行，並負責回收 dead threads |

建議實作步驟：

1. 定義 thread metadata 與 CPU context，確保 C struct layout 與 assembly offsets 一致。
2. 使用 allocator 為每個 thread 配置獨立 kernel stack，並維持 16-byte SP alignment。
3. 實作 `thread_create(entry)`，設定初始 context，讓新 thread 首次執行時進入 entry function。
4. 實作 `switch_to(prev, next)`，保存與恢復 x19–x28、fp、lr 與 sp；使用 `TPIDR_EL1` 保存 current thread 指標。
5. 實作 `schedule()`，從 run queue 選出下一個 thread，再呼叫 context switch。
6. 將 boot context 納入 thread 管理，並建立永遠 runnable 的 idle thread。
7. Entry function 返回或主動 exit 時，將 thread 移出 run queue 並標記為 dead，再切換至其他 thread。
8. 由 idle thread 回收 dead threads；thread 不得在仍使用自己的 stack 時釋放該 stack。

Context switch 只需保存函式呼叫邊界的 callee-saved registers；interrupt 或
exception 打斷執行時所需的完整暫存器狀態，仍由 exception entry 保存。

### Thread Demo

建立 `N > 2` 個 threads，各自執行十次迴圈。每次印出 thread ID 與目前計數，
delay 後呼叫 `schedule()`，應能看到不同 threads 交錯輸出。

所有 threads 返回後，idle thread 應能回收其 stack 與 metadata，並在沒有其他
runnable threads 時繼續執行。

## Goal 2: Kernel Preemption

主動呼叫 `schedule()` 只能完成 cooperative multitasking。若 thread 不主動 yield，
kernel 仍應透過 timer interrupt 取得控制權，切換至其他 runnable threads，完成
kernel preemption。

建議實作步驟：

1. 啟用 core timer interrupt，將 scheduling interval 設為 `counter_frequency >> 5` ticks，約為 1/32 秒。
2. 在 timer interrupt 的安全位置呼叫 scheduler，讓 runnable threads 輪流執行。
3. 整合 scheduling tick 與既有 timeout events，即使沒有 timeout，仍需持續產生 scheduling interrupt。
4. 檢查 run queue、allocator、UART buffers 與 timer/task queues 的 critical sections，只在必要區段暫停 preemption 或 interrupts。
5. 處理 nested IRQ 與 deferred callbacks 的共享狀態，避免在 scheduler 更新一半時再次切換 thread。

Kernel 除了必要的 critical sections 外，應保持可被搶佔，不能以長時間關閉
interrupts 的方式避免多工問題。

### Preemption Demo

讓多個 kernel threads 持續輸出各自的 ID 與計數，不主動呼叫 `schedule()`，
確認 timer 能使它們輪流執行，且結束後仍可正常回收。

## Goal 3: User Process and Basic System Calls

建立在 EL0 執行的 user process，並透過 SVC 進入 kernel 使用 system calls。
每個 process 需要獨立的 kernel stack 與 user stack；進入 exception 時保存 user
context，返回 EL0 前再還原。

System call 使用 `svc 0`，number 放在 x8，參數依序放在 x0、x1、x2 等 registers，
回傳值放在 x0。為了執行課程提供的 Video Player，必須符合以下 ABI：

| Number | System call | 用途 |
| --- | --- | --- |
| 0 | `int getpid()` | 取得目前 process ID |
| 1 | `size_t uart_read(char *buf, size_t size)` | 讀取指定 byte 數至 buffer，回傳實際讀取數量 |
| 2 | `size_t uart_write(const char *buf, size_t size)` | 寫出指定 byte 數，回傳實際寫出數量 |
| 3 | `int exec(const char *name, char *const argv[])` | 執行指定程式，本 Lab 不要求 argument passing |
| 4 | `int fork()` | 複製目前 process，parent 回傳 child ID，child 回傳 0 |
| 5 | `void exit(int status)` | 終止目前 process |
| 6 | `int mbox_call(unsigned char ch, unsigned int *mbox)` | 透過 mailbox 取得硬體資訊 |
| 7 | `void kill(int pid)` | 終止指定 process |

Spec 的 API 說明使用 `exit()`，但測試程式 ABI 使用 `exit(int status)`，實作時
以測試程式 ABI 為準。若完成 advanced signal 的終止機制，spec 允許省略 number 7。

建議實作步驟：

1. 在 thread metadata 中加入 user stack、trap frame 與程式資訊。
2. 擴充 exception frame，保存 x0–x30、SP_EL0、SPSR_EL1 與 ELR_EL1，並同步調整 assembly save/restore layout。
3. 建立初始 user context，設定 entry address、user SP 與 EL0 execution state，再透過 `eret` 進入 user mode。
4. 在 SVC handler 中根據保存的 x8 分派 syscall，從 trap frame 取得參數，並將回傳值寫回保存的 x0。
5. 完成 getpid、UART 與 exit，驗證 syscall 參數、回傳值與 process 結束流程。
6. 調整 EL0 初始 SPSR 的 IRQ mask，驗證 user program 可被 timer interrupt 搶佔。

以簡單的 EL0 程式驗證 getpid、UART 與 exit，再確認不主動 yield 的 user program
仍能與其他 runnable threads 輪流執行。

## Goal 4: Process Operations

利用既有 user process 與 syscall dispatch，加入程式替換、複製、終止與 mailbox
服務。

建議實作步驟：

1. 利用 initramfs loader 實作 exec，取代目前 process 的程式與 user context。
2. 實作 fork，配置 child metadata 與獨立 stacks，複製 user stack 和 context，調整 child SP 與必要的 stack references。
3. 設定 parent 與 child 的 fork 回傳值，讓兩者都從原本 syscall 之後繼續執行。
4. 串接 mailbox 服務，完成 kill 與延後回收，避免已終止的 process 再次被排程。

本 Lab 不啟用 MMU，沒有完整的 process memory isolation。不同程式需使用不同
linker scripts 與載入位址，避免 image 重疊；載入區也不能與 allocator 配置的記憶體
衝突。Fork 程式依 spec 限制，不應使用 global variables、dynamic allocation 或
indirect storage 等會造成共享儲存內容衝突的用法。

Lesson05 的 `copy_process()` 接收外部提供的新 stack，複製 registers 並設定
child SP，但沒有複製 parent user stack 的內容。因此不能直接用它取代本 Lab
的 fork；仍需處理 stack 複製與必要的位址調整，讓 child 從原本 syscall 之後繼續執行。

### Fork Demo

在 EL0 執行 spec 的 fork test 或等價流程，由 parent 建立 child，再由 child
建立 grandchild。印出每個 process 的 PID、local counter、local variable address
與 stack pointer；其中 SP 是 spec 明確要求的輸出。

Parent 應取得 child ID，child 應取得 0。各 process 使用不同的 user stack，
local counter 的變動不應互相覆蓋，結束後則由其他 context 回收資源。

## Goal 5: Video Player Integration

使用課程提供的 Video Player 驗證 user process、syscalls 與 timer preemption
的整合結果。

建議實作步驟：

1. 在 timer 初始化時設定 `CNTKCTL_EL1` bit 0，允許課程程式在 EL0 讀取 physical counter。
2. 載入課程提供的 Video Player user program，以指定的 syscall ABI 在 EL0 執行。
3. 設定顯示輸出，驗證影片播放、shell 輸入與 process 終止能正常運作。

### Video Player Demo


QEMU 需啟用顯示輸出，實體 Raspberry Pi 需接上 HDMI monitor。目前 makefile 的
`qemu` target 使用 `-display none`，後續需調整圖形輸出設定；沿用的 SVC demo
也需換成課程提供的測試程式。

啟動測試程式的 shell 後，輸入 `fork` 建立播放影片的 child。Timer 應持續在
parent shell 與 child 之間切換，讓使用者能在影片播放時繼續輸入命令。

此項必須流暢執行課程測試程式才能取得分數，只有個別 syscalls 正確並不足夠。
若成功完成 Video Player，spec 允許省略 Basic Exercise 1 與 2 的測試展示。

## Goal 6: POSIX Signal

實作非同步 signal delivery，讓 process 能對其他 process 傳送 signal，並執行
default 或 user-registered handler。Default handler 可以在 kernel mode 執行，
user-registered handler 則必須在 EL0 執行。

需要提供：

| System call / Constant | 用途 |
| --- | --- |
| `signal(int SIGNAL, void (*handler)())` | Number 8，註冊目前 process 的 user handler |
| `kill(int pid, int SIGNAL)` | Number 9，對指定 process 傳送 signal |
| `sigreturn()` | Handler 結束後還原原始 user context，number 由實作自行定義 |
| `SIGKILL = 9` | Default action 為終止目標 process |

建議實作步驟：

1. 在 process metadata 中保存 registered handlers、pending signals 與 handler 執行狀態。
2. 實作 signal registration 與 sending APIs，並提供 SIGKILL 的 default handler。
3. 在返回 EL0 前檢查 pending signals，執行 default action 或安排 user handler。
4. 執行 user handler 前保存原始 user context，並配置獨立的 handler user stack。
5. 將返回 EL0 的 entry 設為 handler，並將 LR 指向會呼叫 sigreturn 的 user trampoline。
6. 允許 handler 執行期間發生 syscall 或 interrupt，同時保留原始 context。
7. 在 sigreturn 中還原原始 context、回收 handler stack，讓 process 繼續原本的執行流程。
8. Fork 時複製已註冊的 handlers；本 Lab 不要求處理 nested registered signal handlers。

使用課程測試程式的 `register` 註冊 handler，再使用 `signal_kill {tid}` 驗證
signal 功能。需確認 handler 在 EL0 執行，返回後原本程式仍能正常繼續。
若只通過自行撰寫的 testcase，此項依 spec 最多取得一半分數。

## Build

```bash
make -C Lab5/c
```

## References

- `Lab5/spec.pdf`
- Lab 5 online spec: `https://nycu-caslab.github.io/OSC2024/labs/lab5.html`
- [raspberry-pi-os lesson04](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson04)：kernel threads、context switch 與 timer preemption。
- [raspberry-pi-os lesson05](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson05)：user mode、trap frame、syscall 與 process creation。
