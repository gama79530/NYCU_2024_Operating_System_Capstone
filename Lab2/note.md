# Note

## Basic Exercise 1 - UART Bootloader

一般常見的系統啟動流程，是由 bootloader 直接載入 kernel 並啟動。不過，實務上這個流程可以拆成兩個階段：

1. **Bootloader**：首先執行，負責呼叫 kernel loader 程式。
2. **Kernel Loader**：負責載入 kernel，並將控制權交給 kernel。

這樣設計的好處是：**不需要一開始就把 kernel 放在 non-volatile storage**。  
kernel loader 可以透過各種通訊方式（例如 `UART`）從外部取得 kernel，然後再啟動 kernel，提升系統彈性。

本次任務目標：
- 將上一個 lab 的啟動方式，改為先由 bootloader 啟動 kernel loader，再由 kernel loader 透過 UART 下載 kernel 並啟動。
- 實作 `kernel loader`。
- 實作 host 端的小程式 `kernel updater`，負責將 kernel 從 host 傳送到 Rpi3。
- 需自行設計 kernel loader 與 host 間的通訊協定 (protocol)，確保資料正確傳輸。
- kernel loader 需能載入並啟動下載好的 kernel。

## Basic Exercise 2 - Initial Ramdisk

在作業系統中，檔案操作通常由 kernel 提供的檔案系統支援。但在 kernel 載入初期，檔案系統尚未初始化，因此無法直接存取檔案。  
檔案系統初始化時，可能需要依賴某些硬體驅動程式 (driver)，而這些驅動程式本身也可能需要從檔案系統載入。為了解決這個「先有雞還是先有蛋」的問題，系統會在檔案系統初始化前，先將某些 script 或 binary executable code 載入記憶體。這就是 initial ramdisk（initrd）的用途。

Initial ramdisk 實際上是一個檔案集合，bootloader 會在啟動時將其載入記憶體的特定位置。kernel 在需要時可以解析這個檔案，取得裡面的內容，暫時提供一個檔案系統環境。

本次任務目標：
- 使用 `New ASCII Format Cpio Archive` 格式來實作 initial ramdisk。
- 提供一個 demo function，能夠直接印出 ramdisk 內檔案的內容。

## Basic Exercise 3 - Simple Allocator

一般來說，kernel 會有一個分配器 (allocator) 來管理和分配記憶體空間。這個分配器本身也是一個子系統，為了管理記憶體空間，會產生一些 metadata（例如 allocation table, free list 等），這些資料也需要佔用記憶體空間。因此，kernel 需要一個最基礎的分配器來提供 metadata 的存取空間。

這個任務就是要先實作一個最簡易的分配器。

## Advanced Exercise 1 - Bootloader Self Relocation

一般來說，kernel 並不知道自己會被載入到哪個記憶體位置，而是由 bootloader 直接跳轉到某個位址執行 kernel。在這個 Lab 中，最大的改動是多了一個 kernel loader 。 kernel loader 會佔據原本預設要給 kernel 的記憶體位置。

在 Rpi3 上，可以透過設定檔調整 bootloader 預設跳轉的記憶體位置，並搭配 linker 設定，避免 kernel loader 與 kernel 記憶體重疊。但並非所有硬體都能這樣調整記憶體位址。

本次任務的目標是實作 bootloader 的 self-relocation (自我搬移): kernel loader 會先將自己複製到另一段閒置的記憶體空間，然後根據 offset 跳轉到複製後的新位址，繼續執行後續動作。

## Advanced Exercise 2 - Devicetree

在 kernel 初始化過程中，kernel 必須知道目前有哪些硬體設備連接，以及各個 driver 的位址，才能正確透過 driver 完成初始化。如果硬體連接方式有提供較完整的功能 (如 PCIe 或 USB)，kernel 可以透過對應的 register 自動偵測設備。但在較簡單的硬體架構上，通常沒有這種自動偵測功能。

一種簡單但不理想的做法，是直接將某個記憶體位址對應到特定硬體。但這會讓程式碼與特定硬體綁定，降低可移植性。較好的方式是使用一個專門的檔案來描述硬體資訊，讓 kernel 根據描述自動初始化硬體。這個檔案稱為 `device tree`。

Device tree 有兩種格式
- 給人閱讀和編輯的 `.dts`（Device Tree Source）
- 經過編譯的 `.dtb`（Device Tree Blob），供 kernel 直接讀取。

本次任務的目標是實作一個 parser，能夠解析 `dtb` 檔案。
