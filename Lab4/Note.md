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
| page frame array | 與實體 page frame 一對一對應，記錄每一頁的狀態與 order |
| free list per order | 保存各 order 可直接配置的 block |
| list node in frame entry | 讓 allocator 能在常數時間內插入或移除 free block |

建議實作步驟：

1. 定義 page size、最大 order，以及 frame index 與實體位址的雙向轉換。
2. 定義 frame entry 狀態，至少能區分 block head、隸屬於較大 block 的 free frame、allocated frame 與 reserved frame。
3. 為每個 order 建立 free list，避免配置時掃描整個 page frame array。
4. 實作 allocation：從目標 order 開始尋找 free block；若只能取得較大的 block，逐層 split，並將多出的 buddy 放回對應 free list。
5. 實作 free：由 frame index 與 order 找出 buddy；若 buddy 同 order 且可合併，就從 free list 移除並反覆向上 coalesce。
6. 確保 free-list node 可由 page frame array 直接取得，使每一層 split 或 merge 都是 `O(1)`，整體操作為 `O(log n)`。
7. 印出 allocation、free、split 所釋出的 block，以及每次 merge iteration 的紀錄，供 demo 驗證。

## Goal 2: Dynamic Memory Allocator

Buddy system 適合 page-aligned 或大區塊配置，但小物件若每次都占用整頁會浪費空間。
Dynamic allocator 應以 buddy system 提供的 page frame 作為 backing storage，再將 page
切成固定大小的 chunks。

建議實作步驟：

1. 定義數個常用的 memory pool，例如 16、32、48、96 bytes，並保證回傳位址至少符合 8-byte alignment。
2. 將 request size 向上取整到最接近且足夠容納的 pool size。
3. 在該 pool 尋找 free chunk；若沒有可用 slot，就向 buddy system 申請新的 page frame 並切成 chunks。
4. 配置時標記 chunk 已使用並回傳其位址；釋放時利用同一 page 共享的位址前綴或對應 metadata，找回所屬 pool 與 slot。
5. 對大於最大 pool size 的請求，直接換算需要的 order，交由 buddy system 配置連續 pages。
6. 印出 request size、實際 pool/block size、配置位址與 free 結果，確認 chunk 能被重複利用。

## Goal 3: Reserved Memory

實作可登記任意實體位址範圍的 reserve API，例如
`memory_reserve(start, end)`。被保留的 frame 不得進入 buddy free lists，也不能在 allocator
初始化後被再次配置。

至少需要保留：

| 記憶體範圍 | 原因 |
| --- | --- |
| `0x0000 - 0x1000` | multicore boot spin tables |
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

1. 從 devicetree 的 memory node 取得可用實體記憶體範圍；若暫時不解析，可依 spec 使用 `0x00000000 - 0x3c000000`。
2. 實作簡單的 aligned bump allocator，提供早期配置所需的 `startup_alloc(size, alignment)`。
3. 在 startup 階段持續記錄所有已配置與已保留區域的起始位址及大小。
4. 依實際 frame 數量動態配置 page frame array，不要使用固定大小的靜態陣列。
5. 初始化 buddy system 時，先將所有 frame 視為不可配置，再依 usable ranges 釋出未被保留的區段。
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
