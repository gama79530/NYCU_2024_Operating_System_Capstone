# Lab 6 Note

Lab 6 在 Lab 5 的 threads、scheduler、user processes 與 syscalls 上加入 virtual
memory。Kernel 需要初始化 MMU，建立 kernel 與各 process 的 address space，
再進一步實作 anonymous `mmap()`、demand paging 與 copy-on-write。

本文依本目錄的 `spec.pdf` 整理；各 Goal 是後續實作與驗證計畫，尚未完成。

| Goal | Spec 項目 | 配分 |
| --- | --- | --- |
| 1 | Virtual Memory in Kernel Space | 10% |
| 2 | Virtual Memory in User Space | 30% |
| 3 | Video Player | 40% |
| 4 | Mmap | 10% |
| 5 | Page Fault Handler & Demand Paging | 10% |
| 6 | Copy on Write | 10% |

建議每次只推進一個可驗證的階段：

```text
Lab 5 base
    -> TCR / MAIR 與 identity paging
    -> kernel upper address space 與 RAM / MMIO 屬性
    -> 每個 process 的 page tables 與 address-space switch
    -> exec / fork / mailbox / signal 的 virtual memory 整合
    -> Video Player
    -> anonymous mmap
    -> demand paging
    -> copy-on-write
```

## Base 與目前狀態

`Lab6/c` 複製自 `Lab5/c`，保留 `include/`、`src/` 與 `makefile`。
不複製 `build/`、`bin/` 的既有產物，改由 Lab 6 自行建置。
Header 內的 Lab 5 標記改為 Lab 6，包含 `LAB5_C_` include guards、註解與
診斷文字；其餘程式、assembly、linker layout 與 build rules 維持原樣。
Source 中描述 Lab 5 行為的歷史註解仍保留。

目前繼承的功能包括 allocator、exception handling、timer preemption、kernel
threads、EL0 processes、process operations 與 signals。Base 尚未設定 MMU，
也尚未實作本 Lab 的 address-space isolation、`mmap()`、demand paging 或 COW。

後續修改主要會涉及以下既有檔案；實際介面等推進各 Goal 時再決定：

| 檔案（相對於 `Lab6/c`） | 後續調整重點 |
| --- | --- |
| `src/boot.S`、`src/linker.ld` | MMU 啟動、kernel virtual address 與 early boot |
| `include/arm_v8.h`、`include/peripheral.h` | Translation 定義、MMIO virtual addresses |
| `src/main.c`、`src/buddy.c`、`src/allocator.c` | PA / VA 界線、page-table reservation 與 page frames |
| `src/thread.c`、`src/switch.S` | Process address space、切換、建立與回收 |
| `src/syscall.c`、`src/mailbox.c` | User pointers、syscall ABI 與 mailbox physical buffers |
| `src/exception.c`、`src/exception.S` | Page faults、權限判斷與 context restore |
| `src/el.S`、`src/shell.c`、`include/config.h` | EL0 trampolines、既有 demos 與位址設定 |

### 已知銜接事項

- `src/syscall.c` 的 `SYSCALL_SIGRETURN = 10` 與 `src/el.S` 的 signal-return
  trampoline 使用 syscall 10；本 Lab 指定 `mmap` 為 10。實作 Goal 4 前需重新
  分配內部 `sigreturn` number，並同步 wrapper、dispatcher 與 trampoline。
- Lab 5 的 `fork` 複製 user stack 並調整其中的位址，code/data image 則以
  reference count 共用。Goal 2 要改成相同 user VA 對應不同 PA，建立正確的
  process isolation；單純複製 page tables 而共用可寫 data 不足以完成此目標。
- 目前 EL0 return 與 signal-return trampolines 位於 kernel text。Kernel 移至
  upper address space 並限制 EL0 權限後，需提供 EL0 可執行的 trampoline mapping。
- `CONFIG_USER_STACK_SIZE` 現為 15 KiB allocation，並非 spec 指定的四頁
  virtual stack；`user.img` demo 也仍使用原本的固定低位址。這些留待 Goal 2 調整。
- Makefile 沿用共用的 `BootLoader/bin/initramfs.cpio` 與 DTB；現有 archive
  的測試程式不代表已符合 Lab 6 ABI。Goal 3 前需確認使用 Lab 6 版本的測試程式。

## Background: Address Translation

Spec 固定使用 48-bit virtual address、4 KiB translation granule，不使用 ASID，
並關閉 instruction cache 與 data cache。Lower address space 給 user，upper
address space 給 kernel；kernel 可將可存取的 physical memory 線性映射至
`0xffff000000000000 + PA`。

| 名稱 | 本 Lab 的意義 |
| --- | --- |
| Page | 一段 virtual memory；user mapping 使用 4 KiB |
| Page frame | 一段 physical memory；可存放資料或 page table |
| PGD / PUD / PMD / PTE | 由上而下四層 page tables，每張表占一個 4 KiB frame |
| Block | 由 PUD 或 PMD entry 直接映射的較大區域 |
| `TTBR0_EL1` | Lower address space 的根 page table physical address |
| `TTBR1_EL1` | Upper address space 的根 page table physical address |
| `TCR_EL1` | Address-space 大小與 translation granule 設定 |
| `MAIR_EL1` | 定義 entry 的 AttrIndx 所選用的 memory attributes |

4 KiB granule 的四層 lookup 可依下列 bits 拆解：

| VA bits | 索引／範圍 |
| --- | --- |
| `[47:39]` | PGD index |
| `[38:30]` | PUD index；block 為 1 GiB |
| `[29:21]` | PMD index；block 為 2 MiB |
| `[20:12]` | PTE index；page 為 4 KiB |
| `[11:0]` | Page offset |

Entry 是 64-bit descriptor，包含下一層 table 或目標 frame 的 **physical address**
與 attributes。Page table 要對齊 4 KiB，不能直接用只保證 8-byte alignment
的 `malloc()` 結果充當 page table。可沿用 `buddy_alloc_pages(1, &address)`
取得 page frame，並在使用前清零。

Spec 列出的 descriptor 重點：

| Bits | 意義 |
| --- | --- |
| `[1:0] = 0b11` | 非末層指向下一層 table；末層指向 page |
| `[1:0] = 0b01` | PUD / PMD block descriptor |
| bit 0 = 0 | Invalid entry |
| `[4:2]` | MAIR attribute index |
| bit 6 | 0：僅 kernel；1：user / kernel 可存取 |
| bit 7 | 0：read-write；1：read-only |
| bit 10 | Access flag，未設定會產生 fault |
| `[47:n]` | Output physical address，需依 table / block / page 大小對齊 |
| bit 53 / 54 | PXN / UXN：禁止 EL1 / EL0 執行 |

Spec 特別提醒：AP bits `[7:6] = 0b01` 的 user read-write mapping，即使 PXN
未設定，kernel 也不能在該 mapping 執行。Kernel code 與 user mappings 的權限
必須分開處理。

## Goal 1: Virtual Memory in Kernel Space

依 spec 先做能維持原有 kernel 行為的 identity mapping，再搬到 upper address
space，最後細分 RAM 與 MMIO 的 memory attributes。

建議實作步驟：

1. 設定 `TCR_EL1`：`T0SZ = T1SZ = 16`，`TG0 = 0b00`、`TG1 = 0b10`，
   分別為 48-bit lower / upper VA 與 4 KiB granule。
2. 設定 `MAIR_EL1`：index 0 為 Device-nGnRnE（`0x00`），index 1 為
   Normal non-cacheable（`0x44`）。MMIO 使用前者，RAM 使用後者。
3. 建立 identity paging：PGD 第一項指向 PUD；PUD 前兩項分別映射
   `[0x00000000, 0x40000000)` 與 `[0x40000000, 0x80000000)`。
   此過渡階段依 spec 先全部設為 Device memory，開啟 `SCTLR_EL1.M`。
4. 確認 UART 與基本 kernel 流程仍正常，再調整 linker，使 kernel symbols
   位於 `0xffff000000080000` 起始的 upper address space；實際載入位置仍為
   PA `0x80000`。設定 `TTBR1_EL1`，透過 indirect branch 進入高位址。
5. 檢查 kernel SP、`VBAR_EL1`、DTB、initramfs、allocator、MMIO 與所有
   hard-coded addresses；明確區分 PA 與 kernel VA，避免重複加上 offset。
6. 將 kernel linear mapping 細化為 PMD 2 MiB blocks 或 PTE 4 KiB pages，
   區分 Normal RAM 與 Device MMIO；GPU peripheral 與 ARM local peripheral
   均需有適當 mapping。

Spec 的 early page tables 示範放在 PA `0x0` 與 `0x1000`，屬教學配置。
本專案另有 spin table 與 reserved memory；實作時須確認不重疊，並在 buddy
初始化期間保留實際使用的 page-table frames。

驗證重點：MMU 開啟前後皆能啟動 shell；進入 upper address space 後，UART、
timer、allocator 與 thread switching 仍正常。Device mapping 不允許 unaligned
access，因此「全部 Device」僅作為過渡步驟，不能當作最終 RAM mapping。

## Goal 2: Virtual Memory in User Space

每個 user process 擁有獨立 PGD，kernel mappings 則由 `TTBR1_EL1` 共用。
User mapping 必須使用四層 page tables 與 4 KiB pages。

建議實作步驟：

1. 為 process 建立 address-space metadata，保存 PGD 與 mappings 的所有權。
2. 實作 iteration-based page-table walk；缺少下一層 table 時配置並清零一頁。
3. 實作類似 `mappages(pagetable, va, size, pa, attributes)` 的介面，建立
   user code 與 stack mappings，並檢查對齊、範圍與重複 mapping。
4. 將 user code 映射至 VA `0x0`，stack 使用
   `[0x0000ffffffffb000, 0x0000fffffffff000)`，共四頁（16 KiB）。
   本筆記以 half-open interval 表示 spec 的 stack 範圍，初始 SP 設為上界。
5. Context switch 載入下一個 process 的 PGD **PA** 至 `TTBR0_EL1`，並處理
   barriers 與 TLB invalidation。Spec 示範順序為
   `dsb ish -> msr ttbr0_el1 -> tlbi vmalle1is -> dsb ish -> isb`。
6. 調整 `exec()`：建立新 image、stack 與 address space，成功後才替換舊 context；
   失敗則清理未完成的配置，保留原 process。
7. 調整 `fork()`：parent / child 使用相同 VA，各自具有正確的 private writable
   memory；此階段先採實際複製，Goal 6 再引入 COW，不再掃描與重定位 stack pointers。
8. 調整 user buffer / string 檢查與 `mbox_call()`；user VA 不能直接當作
   peripheral 所使用的 physical buffer address。Mailbox buffer 仍須符合對齊與
   physical layout 要求，可評估 kernel buffer 作為轉接。
9. 整合 signal handler address、獨立 signal stack、saved user context 與
   EL0 trampolines，並處理 exit / kill / exec 後的 page tables 與 frames 回收。

驗證重點：不同 processes 可在相同 VA 保存不同資料；fork 後 stack pointers
仍有效，child 修改可寫資料不影響 parent；exec、preemption、signals 與 mailbox
仍正常，多次建立／結束 process 後記憶體可回收。

## Goal 3: Video Player Integration

使用 **Lab 6 提供的 user program** 驗證前述 address spaces 與所有 Lab 5
syscalls 的整合。Spec 要求測試程式能流暢執行，只有個別 syscall 正確不足以
取得本項分數。

建議實作步驟：

1. 確認 initramfs 內使用 Lab 6 測試程式，並核對載入位址與 syscall ABI。
2. 沿用 `qemu-display` 開啟顯示，驗證影片持續更新，parent shell 同時能操作。
3. 驗證 fork、exec、kill、exit 與 signals；確認 mailbox / framebuffer mappings
   在不同 address spaces 中仍有效，並可在 processes 結束後回收資源。
4. 再於接上 HDMI 的 Raspberry Pi 3 驗證；QEMU 通過不代表實體硬體已驗證。

## Goal 4: Anonymous Mmap

Spec 只要求 anonymous page mapping，不要求 file-backed mapping。
System call number 固定為 **10**；先處理前述 `sigreturn` number 衝突。

```c
void *mmap(void *addr, size_t len, int prot, int flags,
           int fd, int file_offset);
```

| 參數／旗標 | Spec 行為 |
| --- | --- |
| `addr == NULL` | Kernel 選擇未占用的 region 起點 |
| `addr` 對齊且 region 不重疊 | 使用指定起點 |
| `addr` 未對齊或 region 重疊 | 作為 hint，kernel 另選起點 |
| `len` | 向上取整至 page size，建立 page-aligned region |
| `prot` | `PROT_NONE = 0`、`PROT_READ = 1`、`PROT_WRITE = 2`、`PROT_EXEC = 4` |
| `MAP_ANONYMOUS` | Region 對應不屬於檔案的 page frames |
| `MAP_POPULATE` | 立即配置與建立 mapping；spec 允許實作 demand paging 時省略此旗標 |

建議先為 address space 維護 region 起訖與 protection，再處理 frame allocation
與 page-table mapping。Region 不重疊、區間運算不溢位，且 frame 與 page table
的 ownership 必須可回收。驗證指定地址、hint fallback、長度取整與存取權限。

## Goal 5: Page Fault Handler & Demand Paging

將「合法 virtual region」與「已配置 page frame」分開管理。Process 起初只配置
PGD 與必要 metadata，code、stack 與 anonymous regions 的 backing frames
在實際存取時配置；page-table 下層也隨 mapping 需要建立。

建議實作步驟：

1. 在 exception path 辨識 user instruction / data abort，讀取 fault address
   與原因，區分 translation fault 與 permission fault。
2. 若 fault address 不屬於任何合法 region，記錄 segmentation fault 並終止 process。
3. 若位於合法 region 且存取符合 protection，只配置 fault 所需的一頁；
   anonymous page 清零，image page 則依 loader metadata 填入正確內容。
4. 更新 page tables 並完成必要同步，回到原指令重試；不得直接跳過 faulting instruction。
5. 權限違規不能因為 region 存在就放行，也不能把 kernel fault 一律視為可補頁的 user fault。
6. 配置失敗時清理部分資源，避免無限重試 fault 或破壞其他 processes。

Spec 要求每次 fault 有 log，例如：

```text
[Translation fault]: <fault address>
[Segmentation fault]: Kill Process
```

驗證重點：建立大 region 時不立即消耗等量 frames；首次碰觸各頁才配置，
再次存取同頁不重複配置；非法位址或權限違規只終止對應 process。

## Goal 6: Copy on Write

將 Goal 2 的 eager-copy fork 改為共享 backing frames，寫入時才分離。
**Page tables 本身仍需獨立**，不能讓 child 修改 PTE 時直接改到 parent 的表。

建議實作步驟：

1. Fork 複製 page-table pages 與 region metadata，讓 parent / child 暫時共用
   已配置的 user data frames，並增加各 frame 的 reference count。
2. 將原本可寫的 shared pages 在兩邊 PTE 都設為 read-only，保留 region 原始
   write permission 與 COW 狀態，並使 parent 的舊 writable TLB entries 失效。
3. Write permission fault 時，若 region 本來不可寫，視為 segmentation fault；
   若為可寫的 COW mapping，配置新 frame、複製內容、替換 PTE 並恢復 write permission。
4. 更新舊／新 frame 的 reference count 與 TLB；若只剩單一 owner，可評估直接
   恢復 write permission，避免不必要的複製。
5. Exit、exec、kill 與錯誤清理都須減少 references，只有 count 歸零才回收 frame。
6. 特別處理 child 的 mailbox buffer mapping。Spec 明確要求 COW 下 user program
   仍能運作；peripheral 寫入不會觸發 CPU 的 COW fault，不能只依賴 user write faults。

驗證重點：連續多次 fork 可正確共享；任一 process 寫入後資料互不影響；真正的
read-only region 仍受保護；多個 children 依不同順序退出不會提前釋放 shared frames，
且 Video Player / mailbox 在 COW 啟用後仍正常。

## Build

在 repository root 執行：

```bash
make -C Lab6/c
make -C Lab6/c qemu
```

圖形模式與必要時覆寫共用輸入檔：

```bash
make -C Lab6/c qemu-display
make -C Lab6/c qemu INITRAMFS=/absolute/path/initramfs.cpio DTB=/absolute/path/board.dtb
```

預設使用 `aarch64-linux-gnu` toolchain、`qemu-system-aarch64 -M raspi3b`，
initramfs 與 DTB 路徑沿用 `BootLoader/bin/`。以上為操作指令；各 Goal 的完成
狀態與實際驗證結果應在實作後補入，不能以 base 可建置代表 Lab 6 已完成。

### Base 驗證紀錄

- `make -C Lab6/c`：成功產生 `bin/kernel8.img`，無 compiler / linker warnings。
- `make -C Lab6/c qemu QEMU="qemu-system-aarch64 -monitor none"`：成功進入
  shell；`hello` 回覆 `Hello World!`；`thread_demo` 的三個 threads 完成並回收，
  印出 `outstanding: 0`。
- 逐檔比對 52 個 base 檔案：只有 27 個 headers 的 Lab 標記不同，其他內容與
  Lab 5 完全一致；原有 Lab 5 檔案及 Lab 6 spec 的 SHA-256 均維持不變。
- 以上僅驗證繼承的 base；Lab 6 virtual memory 功能與實體 Raspberry Pi 尚未驗證。

## References

- [Lab 6 本地 spec](spec.pdf)：本筆記的需求依據，共 18 頁。
- [Lab 6 online spec](https://nycu-caslab.github.io/OSC2024/labs/lab6.html)：PDF 所列原始課程頁面。
- `../Lab5/Note.md`：繼承程式的設計、既有 demos 與先前驗證紀錄。
