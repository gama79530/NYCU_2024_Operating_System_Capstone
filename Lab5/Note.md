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

排程策略集中在兩個 private functions，後續替換演算法時維持以下介面約定：

```c
static void scheduler_add_thread(thread_t *thread);
static thread_t *scheduler_pick_next(void);
```

`scheduler_add_thread()` 接收 runnable 且尚未連入任何 queue 的 thread，包含 idle；
`scheduler_pick_next()` 從非空 queue 選出下一個 thread 並移出 queue。
Idle 永遠 runnable，確保有候選者。兩者都要求 thread system 已初始化且 IRQ 已遮蔽，不配置記憶體、
不切換 context；前者不失敗，後者不回傳 NULL。

目前採 round-robin：scheduler_add_thread 加到尾端，scheduler_pick_next 從前端取出，
boot、idle 與一般 threads 使用相同規則，不因 zombies 優先選 idle。
`schedule()` 負責 IRQ 保護、判斷 current thread 是否
重新入隊與 context switch；新 thread 也透過同一個 scheduler_add_thread 介面加入排程。

### Thread Demo

Shell 提供 `thread_demo`，建立三個 threads，各自執行十次迴圈。每次印出 thread
ID、目前計數與 SP，delay 後呼叫 `schedule()`，可觀察 round-robin 的交錯輸出。
Runner 自然返回後，由 `thread_start()` 自動呼叫 `thread_exit()`，不需在 runner
內自行處理 thread 結束。

```text
$ buddy
$ thread_demo
$ buddy
```

初版保留 shell 為 boot thread（ID 0），idle 為 ID 1，動態建立的 threads 從 ID 2
開始，退出後 ID 可重用。每個動態 thread 透過 `malloc/free` 配置與釋放 15 KiB
stack 空間，不額外配置 alignment 預留。`stack_allocation` 保存原始位址供 free
使用，`stack_bottom` 向上、`stack_top` 向下對齊 16 bytes，初始 SP 設為 top。
設定值代表配置大小；依目前 allocator 的 8-byte alignment 保證，實際可用容量
可能減少 16 bytes。以目前 4 KiB pages 計算，含 allocation header 仍占四頁。
Run queue 保存等待執行的 threads，包含等待中的 boot 與 idle，但不包含 current。
初始化時 boot 正在執行，只將 idle 入隊；boot 呼叫 `schedule()` 時才重新入隊。

Idle 的 metadata 維持靜態，stack 則在 `thread_init()` 中透過 dynamic allocator
配置，使用與一般 thread 相同的大小與 SP 對齊方式，並保留至 kernel 結束。
配置失敗時 `thread_init()` 回傳 false，不啟用 thread system。
Idle 參與一般 round-robin，每輪最多回收 `CONFIG_THREAD_REAP_LIMIT` 個 zombies，
預設為 1，且每回收一個才遮蔽 IRQ，完成後還原，再處理下一個。
即使還有 zombies，達到上限後仍呼叫 `schedule()` 讓出 CPU；沒有 zombies 時直接 yield。
Demo 等到 idle 回收全部 stack 與 metadata 後才返回 shell，並印出 `outstanding: 0`；
前後兩次 `buddy` 的 free page 總數應相同。

目前是 cooperative scheduling：必須主動 yield，且不得從 IRQ、deferred callback
或 EL0 呼叫 `schedule()`。Thread 建立失敗時回傳 -1，已建立的 threads 仍會執行
並回收；boot 與 idle 不可呼叫 `thread_exit()`。

ID 容量由 `CONFIG_THREAD_ID_COUNT` 設定，預設 256 slots，扣除 boot／idle 後
可供 254 個尚未退出的 threads 使用。`free_id_next[]` 以 index 串接 free IDs：
`[0]` 保存 head、`[1]` 保存 tail，鏈結以 1 結尾。取空後 head 為 1，此時 tail
沒有意義，不應讀取。歸還時若 list 為空就設定 head，否則接到舊 tail，最後更新 tail。
配置與歸還皆為 O(1)；256 slots 以下使用 `uint8_t`，更大容量使用 `uint16_t`。

ID 在 thread 退出時歸還到 tail，zombie 的 ID 設為 -1；idle 只依 pointer 回收
stack 與 metadata，不會再次歸還 ID。建立途中若配置記憶體失敗，也會歸還已取得的
ID。目前沒有 wait 語意，因此 ID 釋放與記憶體回收分開處理。

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

保留 `thread_demo` 每輪主動呼叫 `schedule()` 的方式，觀察 timer preemption 與
yield 同時運作時，各 thread 的輸出順序變化，並確認結束後仍可正常回收。
另外以暫時測試驗證不主動 yield 的 threads 仍能輪流執行，不增加額外 shell 指令。

Scheduling tick 使用 periodic event，只標記重新排程需求；最外層 IRQ 收尾後才切換，
確保 timer 已重新設定、deferred callback 的共享狀態已還原。Goal 2 階段先只搶佔 EL1 kernel
threads；IRQ/deferred callbacks 中途不切換 thread，因此長 callback 仍會延後搶佔。
每次 tick 的間隔從 callback 返回後計算，允許漂移，不補跑錯過的週期。

這是目前選擇的 scheduling policy，不是 ARM 或 context switch 必須遵守的規則。
允許 nested IRQ，只代表 handler 能被另一個 IRQ 打斷，不代表它已能安全地與其他
threads 交錯執行。`reschedule_pending` 記錄尚未處理的排程需求，`exception_depth`
計算 SYNC、IRQ、FIQ、SError 的 nesting，但排除 user 同步 exception。
其餘 handlers 預設必須完成才允許切換；若內層 IRQ 收尾時 depth 尚未歸零，
就保留排程需求，等後續符合條件的 IRQ 收尾再切換。

採用這個 policy 是為了沿用目前 `task_queue` 的全域 `task_running` 與
`current_task_priority`：它們假設 callback 依巢狀順序執行、返回並還原狀態。
若在內層 IRQ 收尾時切換 thread，外層 callback 尚未還原的狀態就可能影響另一個
thread 的 deferred task 執行。代價是長 callback 會延後 thread 搶佔，因此 callback
應保持短小。

也可以選擇允許 callback 中途被 thread 搶佔，但必須重新設計共享狀態與同步方式，
不能只移除 depth 判斷。另一個方向是把耗時工作交給 kernel worker thread，讓
硬體 IRQ 處理保持短小，後續工作再由 scheduler 排程；不需要所有 IRQ 衍生工作
都完成後才能切換。至於在 timer callback 直接呼叫 `schedule()` 前，必須先安排好
下一次 timer，這是另一個獨立的執行順序問題，不是 depth 判斷本身的理由。

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

### 初版驗證：fork_test

先使用內建的 `fork_test` 指令，在 EL0 執行 spec 的等價測試，暫不整合外部 user
image。測試只使用 local variables，透過 `svc 0` 呼叫 getpid、uart_write、fork 與
exit，印出 parent、child、grandchild 的 PID、counter、local address 與 SP。
因此提前實作 Goal 4 的最小 fork；其餘 syscall 接著由 Goal 4 補齊。

每個 user thread 有獨立的 allocator-owned kernel/user stack。`thread_create()`、
`thread_create_user()` 與 `thread_fork()` 都採「配置資源 → 準備 context → 加入
run queue」的流程，準備完成前不會被排程。配置與失敗清理使用共用 helper；
發布則統一透過 `thread_publish()` 加入 queue 並更新數量。

`thread_create_user()` 配置新的 metadata、ID、kernel/user stacks，在尚未執行的
kernel stack 頂端直接準備初始 EL0 frame；成功回傳 ID，失敗清理資源並回傳 -1。
Fork 共用 user stack 與 frame/start context 的配置，中間改為複製 parent stack
與 frame、調整位址及回傳值，最後才發布 child。沒有轉換當前 kernel runner 的
介面，也不需要額外搬移初始 frame；user exit 結束整個 thread。

Exception frame 原本的 padding 改為保存 `SP_EL0`，與其他 registers 一起還原。
初次執行與 fork child 都從準備好的 frame 透過 `eret` 進入 EL0；entry 返回時由 EL0 trampoline
呼叫 exit。Idle 同時回收兩種 stack。已結束 process 的 stack 位址可能被後來的
process 重用，所以不同時間印出的相同 SP 不一定表示它們同時共用 stack。

EL0 syscall 屬於可排程的 thread context，不計入 `exception_depth`，因此
UART 等待時其他 threads 仍能執行，exit 不返回也不會留下未歸零的 depth。
User 同步 fault 也不計入，因為目前無法修復時會直接結束 thread。
真正的 IRQ/deferred callbacks 仍採 Goal 2 的最外層收尾切換 policy。

這個雛型沒有 MMU 或完整 memory isolation。Fork 複製使用中的 user stack 與
trap frame，並將 registers、stack 中落在原 stack 範圍內的值搬到 child 對應位置，
包含 saved FP chain。它無法分辨同值整數與 pointer，也不處理未對齊或存在其他
記憶體中的 pointers，不是通用的 POSIX fork。內建測試的 code/唯讀資料沿用
kernel image；外部程式則共用 loader 配置的 image。UART syscall 接受目前
user stack 或 loaded image 內的 buffer。exit 接受 status，但沒有 wait API，
暫不保存 status。Unsupported syscall 回傳 -1。

### 已知現象：UART 輸出交錯

實體 Raspberry Pi 執行 `fork_test` 時，觀察到不同 threads 的輸出在同一行中途
交錯，包含 user 的 UART write 與 boot thread 的 `printf()`；目前 QEMU 測試
尚未重現。UART 逐字輸出期間允許 timer preemption，因此一次輸出並非 atomic，
此現象本身不代表 fork context 錯誤。測試的等待迴圈維持 `schedule()` 主動 yield。

先記錄並擱置輸出同步。若後續處理，需讓 kernel/user writers 共用序列化機制，
並另行處理 IRQ context 的輸出；不能讓 IRQ 等待被它打斷的持鎖 thread。
不以整段關 IRQ 的方式保護輸出，以免延後 preemption；read 也不應共用可能
長時間等待的輸出鎖。

## Goal 4: Process Operations

利用既有 user process 與 syscall dispatch，加入程式替換、複製、終止與 mailbox
服務。

建議實作步驟：

1. 利用 initramfs loader 實作 exec，取代目前 process 的程式與 user context。
2. 實作 fork，配置 child metadata 與獨立 stacks，複製 user stack 和 context，調整 child SP 與必要的 stack references。
3. 設定 parent 與 child 的 fork 回傳值，讓兩者都從原本 syscall 之後繼續執行。
4. 串接 mailbox 服務，完成 kill 與延後回收，避免已終止的 process 再次被排程。

### 目前實作

Basic syscall numbers 0–7 已有 handler；signal 的 8、9 與 sigreturn 的 10 見 Goal 6。

`thread_create_program(name)` 從 initramfs 找出 image，配置新 thread 與 stacks，
載入 image 並準備初始 frame，最後才發布到 run queue。Kernel shell 的
`syscall_demo` 使用這個介面固定載入 `syscall.img`，建立新 user process；不接受路徑參數。
Demo 只允許 boot thread 在沒有其他動態 threads 時啟動，等待初始 user shell
結束期間主動 yield，不讀 UART。初始 process 結束後，終止仍存活的 user children，
再等 idle 回收全部資源才返回 kernel shell，避免未先 kill 的影片 children 阻止返回。
`exit` 本身仍只終止呼叫者；這是 demo 的收尾策略，不另引入 process group。
初始 process 以 ID 查找是否存活；目前影片 children 不會再 fork，因此初始 shell
結束後，其 ID 不會在等待收尾期間被重新配發。原有 `svc` 仍是不經 thread lifecycle
的舊 SVC 診斷入口。

User 的 `sys_exec(name, argv)` 則替換當前 process，保留 PID 與 kernel stack。
先載入新 image，成功後才重設 user stack 與 active syscall frame；exception 返回
assembly 的 `eret` 會進入新 entry，不回到原本的 syscall 呼叫點。
找不到檔案或配置失敗回傳 -1，原程式與 stack 保持可繼續執行。
本 Lab 不要求 argument passing，因此 `argv` 先保留但不使用。

Loader 使用 allocator 配置 image，entry 固定為檔案的第一條 instruction。
目前接受 flat、可搬移的 binary，並將 entry 對齊到 4 KiB，保留 `ADRP` 的
page-relative references。程式不能依賴固定的 absolute load address，也不做
ELF parsing 或 relocation；BSS/storage 必須已包含在 image 中，或由程式自行
在其有效範圍內初始化。複製後同步 D-cache 與 I-cache，再允許執行。

Image metadata 保存 reference count。Fork 共用 parent image，獨立複製 stack
與 frame；parent exec 或退出後，child 仍保有舊 image。最後一個 reference
釋放時才回收 image。沒有 MMU 或完整 isolation，fork 程式仍依 spec 限制，
避免 global variables、dynamic allocation 與 indirect storage 的共享內容衝突。
若要載入依賴固定 link address 的程式，需另行支援保留載入區或 executable
relocation，不能直接搬到 allocator 任意選出的地址。

`mbox_call` 支援 property channel 8。先確認 user buffer 的 16-byte alignment、
header 宣告的 byte size 與完整 user range，再檢查 tag lengths/end tag。
Mailbox 模組使用獨立 16-byte-aligned copy，送出前與回應後保留 memory barriers，
將 firmware response 複製回原 buffer，成功回傳 1，mailbox 失敗
回傳 0，無效 user range/channel 回傳 -1。若送出後回應逾時，保留該 copy，
後續 call 收到遲來回應才釋放，避免 firmware 寫入已回收的記憶體。
目前 Lab5 維持 MMU 關閉，mailbox 不做 D-cache maintenance；後續 Lab 啟用
MMU 並將 buffer 設為 cacheable 時，再加入 cache clean/invalidate。

`kill(pid)` 只終止 user process，保護 boot、idle 與 kernel threads。
目標從 run queue 移除、釋放 ID，加入 zombie queue，交給 idle 回收 image
reference、user/kernel stacks 與 metadata。Kill 自己直接沿用 `thread_exit()`；
無效或已結束的 ID 不影響其他 threads。IDs 仍可重用，沒有 generation check。

目前 image 載入/exec replacement 與 bounded mailbox transaction 會暫時 mask
IRQ。前者避免 kill 中斷尚未發布的配置造成洩漏，後者避免不同 threads 搶走
彼此的 mailbox FIFO response。這是此版本簡化同步的取捨；較大的 image 或
mailbox timeout 會延後 timer preemption。UART 等待仍允許 IRQ/preemption，
輸出交錯維持先記錄、暫不序列化的策略。

### 驗證

使用暫存的外部 EL0 images 在 QEMU 驗證 syscall 0–7，包含 UART image buffer
與 read、invalid pointers、mailbox revision/invalid tags、exec 失敗與成功、
PID 保留、fork image 共用、kill 停在 UART read 的 child，以及 self-kill。
預設與 `-O2` build 各重複三輪，回到 kernel shell 且 buddy 狀態與測試前一致；
既有 `thread_demo`、`fork_test` 與 `Lab5/user/bin/user.img` 也正常結束。
Mailbox 的逾時/遲來回應路徑另以模擬 MMIO 驗證：逾時保留單一 allocation，
未收到回應前不發新 request，收到遲來回應後回收，write timeout 則直接釋放。
Firmware timeout recovery 尚未在實體 Raspberry Pi 驗證。

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

QEMU 使用 `make -C Lab5/c qemu-display` 開啟顯示視窗，UART 仍在原終端機操作。
啟動後輸入 `syscall_demo` 執行課程提供的 `syscall.img`；此 target 不暫停 CPU，
不需要先連線 GDB。原有 `qemu` target 維持 `-display none`。
實體 Raspberry Pi 需接上 HDMI monitor。

啟動測試程式的 shell 後，輸入 `fork` 建立播放影片的 child。Timer 應持續在
parent shell 與 child 之間切換，讓使用者能在影片播放時繼續輸入命令。

`timer_init()` 設定 `CNTKCTL_EL1.EL0PCTEN`，允許 EL0 讀取 `CNTFRQ_EL0` 與
`CNTPCT_EL0` 來計算影片幀間隔，並用 `isb` 同步權限變更；timer 控制仍限於 kernel。
若缺少這項設定，child 會在 image offset `0x10` 的 `mrs x0, cntfrq_el0` 觸發
`ESR_EL1 = 0x6230f801`（EC `0x18`，system register access trap）。

QEMU 圖形模式已重複驗證兩輪 `syscall_demo → fork → kill child → exit`。
每輪擷取三張內容不同的 1024×768 framebuffer，確認畫面持續更新；播放期間
parent 的 `pid` 仍有回應，kill 後畫面停止更新，exit 返回 kernel shell。
既有 `fork_test` 與 `thread_demo` 也通過；影片播放尚未在實體 Raspberry Pi 驗證。

另驗證多次 fork 後不先 kill 就直接 exit、自行 kill parent，以及 parent exec
後再 fork/exit；皆會清理剩餘 children 並返回 kernel shell，framebuffer 停止更新，
buddy 狀態恢復測試前的數值。一般 `fork_test` 的 child 仍可在 parent 結束後繼續執行。

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
| `sigreturn()` | Number 10，handler 結束後還原原始 user context |
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

### 目前實作

`sys_sigregister(signum, handler)`、`sys_sigsend(pid, signum)` 與 `sys_sigreturn()`
分別包裝 syscall 8、9、10；原有 syscall 7 的 `sys_kill(pid)` 仍可直接終止 process。
註冊與傳送成功回傳 0，無效 signal、PID 或 handler 回傳 -1；沒有 active handler
時呼叫 sigreturn 也回傳 -1。NULL handler 表示恢復 default action。

`CONFIG_SIGNAL_COUNT` 預設為 32，支援 1–31。沿用 POSIX 將 0 保留給
`kill(pid, 0)` 的目標/權限查詢語意，不將其當成一般 signal；目前 Lab 尚未實作
此查詢，傳入 0 仍回傳 -1。每個 thread 保存
handler table、pending bitmap 與可選的 active signal context，同號 pending signal
合併成一個。未註冊的 SIGKILL 在 kernel 直接終止目標，即使目標卡在 UART read
也有效；其他未註冊的 signals 暫時忽略。依本 Lab 與課程程式要求，signal 9 可以
被註冊的 user handler 覆寫，這點不同於一般 POSIX 的不可攔截 SIGKILL。

所有 exception restore（包含初次進入 EL0 與 fork child）在還原 registers 前，
統一呼叫 `thread_signal_deliver()`。只處理即將返回 EL0 的 frame，EL1/nested IRQ
返回不插入 handler。Registered signal 若在 syscall 執行期間抵達，會等到該
syscall 返回 EL0 才執行，不中斷或重啟 blocking syscall。

Delivery 在 IRQ masked 時保存完整 frame 到 kernel-owned signal context，另外
用 allocator 配置 `CONFIG_SIGNAL_STACK_SIZE`（預設同 user stack，15 KiB），將
PC 改為 handler、SP 改為新 stack、LR 指向 `el0_signal_return`；x0 也帶上 signum，
但 spec 的 `void handler(void)` 可以不使用它。Handler 在 EL0 執行，可呼叫 syscall
並被 timer 搶佔；UART 等 buffer 檢查也接受 active handler stack。

Handler 返回後，trampoline 發出 sigreturn。Kernel 從自己保存的 frame 還原
x0–x30、SP、PC 與 SPSR，並釋放 handler stack/context；不把 syscall 的回傳值
覆蓋到剛還原的 x0。Active handler 執行期間不再進入下一個 registered handler，
新的 signal 保留 pending，等 sigreturn 完成後再處理。配置 context/stack 失敗時
直接終止接收者，避免以不完整狀態執行 handler。

Fork 複製 registered handlers，但不繼承 pending bitmap。若在 handler 內 fork，
也複製 active handler stack、原本的 normal stack 與 saved frame，沿用既有的
stack-range relocation，讓 parent/child 都能各自 sigreturn。成功 exec 清除舊的
handlers、pending 與 active context；exit、kill 與 demo 收尾則由 idle 一併回收。
Handler entry 要求 instruction alignment 且位於 loaded image（內建測試則是
kernel text）；沒有 MMU，這仍不是完整的 memory isolation 或通用 POSIX signal。

### Signal Demo 與驗證

```text
$ syscall_demo
# fork
# signal_kill <child_pid>
# register
# fork
# signal_kill <new_child_pid>
# kill <new_child_pid>
# exit
```

第一次 signal_kill 使用 default action 終止 child；register 後建立的 child
繼承 handler，會印出課程訊息，等待後返回繼續播放。若希望直接終止這個 child，
使用原有的 kill。Parent 退出時 demo 仍會清理剩餘 children。

已使用原始課程 binary 在 QEMU 圖形模式驗證 default action、fork inheritance、
handler 中的 syscall/preemption、重複 delivery、sigreturn 後影片繼續更新，以及
直接 kill active handler 的回收；QMP registers 也確認 handler 在 EL0t 執行。
另以暫存的外部測試 images（`-O0`/`-O2`）驗證完整 context/NZCV 還原、獨立 stack、
pending 合併與不巢狀、無效參數、handler 內 fork/exec/exit，重複執行後 buddy
狀態一致。實體 Raspberry Pi 的 signal 功能尚未驗證。

## Build

```bash
make -C Lab5/c
```

## References

- `Lab5/spec.pdf`
- Lab 5 online spec: `https://nycu-caslab.github.io/OSC2024/labs/lab5.html`
- [raspberry-pi-os lesson04](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson04)：kernel threads、context switch 與 timer preemption。
- [raspberry-pi-os lesson05](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson05)：user mode、trap frame、syscall 與 process creation。
- [Mailbox property interface](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface)：channel 8、buffer alignment 與 property tags。
- [Arm: Implementing clear cache](https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/caches-self-modifying-code-implementing-clear-cache)：複製 instructions 後的 D-cache/I-cache 同步。
