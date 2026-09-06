# Lab 4 Note

Lab 4 要建立 kernel 後續功能會共用的記憶體配置系統。Spec 原本分成兩個 basic
exercise 與三個 advanced exercise；這裡將 Buddy System 與 Efficient Page Allocation
合併，整理成四個相互銜接的實作目標：

1. Efficient Buddy System
2. Dynamic Memory Allocator
3. Reserved Memory
4. Startup Allocator

初始化流程建議如下：

```text
取得實體記憶體範圍
    -> 啟用 startup allocator
    -> 登記 kernel、initramfs 等 reserved ranges
    -> 動態配置 page frame array
    -> 初始化 buddy system
    -> 將未保留的實體記憶體交給 buddy system
    -> 啟用 dynamic memory allocator
```

## Goal 1: Efficient Buddy System

實作以 4 KB page frame 為單位的 buddy system，支援配置連續的
`2^order` 個 page frames。最大 order 必須大於 5，並應同時完成 Advanced Exercise 1
的效率要求，使配置與釋放操作維持在 `O(log n)`。

建議資料結構：

| 資料結構 | 用途 |
| --- | --- |
| singleton buddy metadata | 集中保存 managed range、frame array 與各 order free area |
| 1-byte frame array | 以 2-bit state 與 6-bit order 記錄每個實體 page frame |
| free area per order | 保存 free-list head 與 free-block count |
| list node in free page | 使用 free block 的開頭保存 list node，不擴大 per-frame metadata |

建議實作步驟：

1. 使用 `BUDDY_MAX_ORDER` 表示 inclusive maximum，並以 `BUDDY_ORDER_COUNT` 表示 array 長度。
2. 將 allocator control state 收進 singleton struct；frame array 仍依 managed RAM 大小動態配置。
3. Frame 預設為 free candidate，build 前將已使用區域與 memory holes 標成 reserved。
4. Build 由小到大合併 free frames，建立 maximal blocks，再加入各 order free lists。
5. Public allocation API 接收 page count；內部找到最小可用 order，保留連續 prefix，並將多餘 suffix 拆解後歸還 free lists。
6. Free 接收原始 address 與 page count，先驗證所有 allocated pieces，再以 XOR 反覆 merge。
7. Split、merge 與 address conversion 保持 private；public API 只回報可預期的 input、state 與 OOM errors。
8. 印出 allocation、free、split 所釋出的 block，以及每次 merge iteration 的紀錄，供 demo 驗證。

## Goal 2: Dynamic Memory Allocator

Buddy system 適合 page-aligned 或大區塊配置，但小物件若每次都占用整頁會浪費空間。
Dynamic allocator 應以 buddy system 提供的 page frame 作為 backing storage，再將 page
切成固定大小的 chunks。

建議實作步驟：

1. 依 buddy page size 產生 8、16、32、48、64、96 bytes 等 memory pools，最大 small chunk 為 page size 的 3/8，並保證回傳位址至少符合 8-byte alignment。
2. 將 request size 向上取整到最接近且足夠容納的 pool size。
3. 每個 pool 維護仍有 free chunks 的 page list；若沒有可用 page，就向 buddy system 申請新的 page frame 並切成 chunks。
4. Free chunk 本身保存下一個 free chunk 的 pointer，使小區塊配置與釋放不需要掃描 bitmap。
5. 釋放時利用同一 page 共享的位址前綴找回 page header；page 完全空閒時將它歸還 buddy system。
6. 對大於最大 pool size 的請求，將 allocation header 與 payload 換算為 exact page count，再交由 buddy system 配置連續 pages。
7. Timer events 與 task nodes 統一透過 `malloc()`、`free()` 配置，同時保留具有獨立容量上限的 local object cache。
8. Startup allocator demo 與 buddy metadata 保留直接使用 `simple_malloc()`，其他 runtime allocations 則使用統一 allocator API。
9. 印出 request size、實際 pool/block size、配置位址與 free 結果，確認 chunk 能被重複利用。

### Allocator Demo

Shell 提供 `malloc <bytes>` 與 `free <address>`，可直接用 allocator 回傳的位址
配置及釋放 dynamic memory：

```text
$ malloc 64
malloc: allocated 64 bytes at 0x0018DFA8
$ malloc 6000
malloc: allocated 6000 bytes at 0x00002010
$ free 0x0018DFA8
$ free 0x00002010
```

位址會隨當下 memory layout 改變，`free` 時應使用同一次 `malloc` 實際印出的位址。
這是低階 demo interface，不追蹤 allocation，也不防止錯誤位址或重複釋放。

`kmem_demo` 會先說明配置與釋放順序，再自動執行一組完整範例。若要看到 buddy
內部的 split/coalesce 過程，先將 `CONFIG_VERBOSE` 設為 `1` 並重新建置；關閉時
仍會執行相同範例，但只顯示 demo 摘要。

Fresh boot 進入 shell 後可在執行前後印出 buddy 狀態：

```text
$ buddy
$ kmem_demo
$ buddy
```

執行時，每一階段都會先印出即將模擬的 shell command；重複的 small allocation
與 release 會各自整理成一個 log 區塊，避免 allocator 訊息彼此混淆。

這組範例分成三個階段：

1. 配置 13 bytes。13 並未對齊，allocator 應選擇 16-byte pool，且回傳位址仍須符合 8-byte alignment；確認後立即釋放。
2. 連續配置三個最大 small-pool chunks。預設 4 KiB page 下，每個 chunk 是 1536 bytes，而一個 pool page 只能容納兩個，因此第三筆配置會建立第二個 pool page。三筆配置完成後再反向釋放，應看到兩個 pool pages 分別歸還 buddy。
3. 連續配置六個 order 9 large areas，再反向釋放。每筆 payload 使用 `(2^9 - 1) * page_size`，加上 large header 後剛好需要 512 pages。依 fresh boot 的 free-list 狀態，前五筆會消耗既有 order 9 block 與兩個 order 10 blocks，第六筆則迫使 order 11 連續 split 成 order 10、order 9；反向釋放時會連續 coalesce 回 order 11。

| Verbose 訊息 | 代表動作 |
| --- | --- |
| `create ... pool page` | Dynamic allocator 向 buddy 取得一頁並切成 small chunks |
| `release empty pool page` | Pool page 的 chunks 全部釋放，整頁歸還 buddy |
| `allocate 13 bytes` 搭配 `create 16-byte pool page` | 未對齊 request 被向上選入可容納它的 size class |
| `create 512-page large block` | Large allocation 取得一個完整 order 9 buddy group |
| `release 512-page large block` | Order 9 large allocation 已完整歸還 buddy |
| 連續的 `split free ... order N` | 較高 order buddy group 正逐層拆成較小 blocks |
| 連續的 `merge ... order N` | 相鄰 free buddies 正逐層合併回原本的大 block |

第二次 `buddy` 的 free page 總數應與第一次相同，表示 demo 使用的 pool pages 與
large area 都已完整歸還。

## Goal 3: Reserved Memory

實作可登記任意實體位址範圍的 reserve API，例如
`memory_reserve(start, end)`。被保留的 frame 不得進入 buddy free lists，也不能在 allocator
初始化後被再次配置。

至少需要保留：

| 記憶體範圍 | 原因 |
| --- | --- |
| `0x0000 - 0x1000` | multicore boot spin tables |
| kernel stack | 由 linker symbols 定義的 downward-growing boot stack |
| kernel image | kernel text、rodata、data 與 bss 正在使用 |
| initramfs | kernel 後續仍需存取封存內容 |
| devicetree | 若初始化後仍會使用 FDT，應一併保留 |
| startup allocator 已使用區域 | 包含 page frame array 與其他早期配置 |

建議實作步驟：

1. 設計 reserved-range 紀錄格式與 `memory_reserve(start, end)` API。
2. 將輸入範圍向下及向上對齊至 page boundary，並處理重疊或相鄰 ranges。
3. 從 linker symbols 取得 kernel image 範圍，從 FDT 取得 initramfs、devicetree 與實體記憶體資訊。
4. 在 buddy 初始化前登記所有 reserved ranges。
5. 建立 free blocks 時跳過任何與 reserved range 重疊的 frame，以支援 memory holes。
6. 提供 reserved-range 與 frame 狀態的 log，確認保留區不會被配置。

## Goal 4: Startup Allocator

Page frame array 的大小取決於執行時取得的實體記憶體容量，但它又必須在 buddy system
啟動前完成配置。Startup allocator 用來打破這個相依循環：先用不依賴 page allocator
的簡單配置方式供早期初始化使用，再把剩餘記憶體交給 buddy system。

建議實作步驟：

1. 從 devicetree 的 root `#address-cells`、`#size-cells` 與 memory node 的 `reg` 取得可用實體記憶體範圍；解析失敗時依 spec fallback 至 `0x00000000 - 0x3c000000`。
2. 實作簡單的 aligned bump allocator，提供早期配置所需的 `startup_alloc(size, alignment)`。
3. 在 startup 階段持續記錄所有已配置與已保留區域的起始位址及大小。
4. 依實際 frame 數量動態配置 page frame array，不要使用固定大小的靜態陣列。
5. 初始化 buddy system 時，依 managed range 建立 frame array，再將 reserved ranges 與 memory holes 排除。
6. 將 kernel、initramfs、spin tables 與 startup allocator 使用範圍透過 reserve API 標記為 allocated。
7. Buddy system ready 後停止一般 startup allocation，後續配置改走 buddy system 或 dynamic allocator。

Buddy system 必須能處理大型記憶體範圍與 memory holes，且所有未保留的 usable memory
都應納入管理。

## Suggested Implementation Order

1. 定義 page frame entry、free lists 與 allocator public APIs。
2. 完成 reserve API 和 reserved-range 紀錄。
3. 完成 startup allocator，並取得 usable memory 與必要的 reserved ranges。
4. 動態配置 page frame array，初始化 efficient buddy system。
5. 驗證不同 order 的 allocate、split、free 與 iterative merge。
6. 在 buddy system 上建立 chunk-based dynamic allocator。
7. 加入 shell demo command 與完整 logs，驗證 reserved pages、memory reuse 及配置效率。

## Build

```bash
make -C Lab4/c
```

## References

- `Lab4/spec.pdf`
- Lab 4 online spec: `https://nycu-caslab.github.io/OSC2024/labs/lab4.html`
