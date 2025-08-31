# Note

## Basic Exercise 1 - Basic Initialization

這個部份需要完成 4 個啟動 kernel 之前的準備動作

- 所有的資料要放到正確的位址
- Program Counter 要指向正確的位址
- bss 區段有被正確的初始化為 0
- 有將 stack pointer 指向適當的位址

前兩個要求 Rpi3 會自動把 image 放到 0x80000 並將 PC 指向該位址。
需要通過程式處理的是後面兩個部份。

## Basic Exercise 2 - Mini UART

Rpi3 有兩種 UART 可以使用，分別是 `mini UART` 與 `PL011 UART` 。 這個作業的要求是要把使用 mini UART 要做的初始化完成，並提供通過 mini UART 進行讀與寫的 API 。

Rpi3 的 CPU 在與週邊設備進行 IO 時使用的是機制是 `MMIO (memory mapped input output)` 。 當在特定的記憶體位址讀寫時，其實就是在讀寫週邊設備的 register 的值。

### Bus address v.s. Physical address

在 `SoC` 中，不同的設備是透過 bus 來進行通訊。為了方便文件表達，所有設備的 MMIO 位址會被統一映射到 `bus address space`。文件上所紀錄的地址也是使用這個 address space。  

然而，CPU 在實際存取記憶體時使用的是 `physical address space`。因此在撰寫程式時，需要將文件上的 bus address 轉換為對應的 physical address。不同的 SoC 或 CPU 其轉換方式可能有所不同，必須在開發前先確認。  

※ 在 RPi3 上的轉換方式是：將位址開頭的 `0x7e` 改為 `0x3f`。

## Basic Exercise 3 - Simple Shell

要實做一個簡單的 Shell 並提供 `help` 與 `hello` 兩個 command

## Basic Exercise 4 - Mailbox

在 RPi3 架構中，部分週邊設備 (如 framebuffer、時脈、電源管理等) 由 `VideoCore (GPU/firmware)` 管理， CPU 不能直接控制這些週邊。為了讓 CPU 與 GPU 溝通，RPi3 提供 Mailbox 機制，CPU 透過將控制指令傳送給 GPU，由 GPU 來執行相關的硬體設定。

這個作業的要求是要利用 Mailbox 的功能來取得硬體資訊。

## Advanced Exercise 1 - Reboot

RPi3 不提供實際的 reset 按鈕，這個作業的要求是實做 `reset` 指令。