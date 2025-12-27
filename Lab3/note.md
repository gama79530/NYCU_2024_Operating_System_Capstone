# Note

## Basic Exercise 1 - Exception

這個 erercise 主要的目的是要學會怎樣在 exception 之間跳轉，因此實際上沒辦法與其他 exercise 完全切開。

在 ELx 的時候如果想要跳轉 EL 通常是通過 `eret` 這個手段來實現。
在呼叫 eret 的時候 Rpi3 會自動根據 

- `spsr_elx` 跳轉過後的相關設定，其中最重要的部份是有包含跳轉之後的 EL
- `elr_elx` 判斷要跳轉到哪個地方執行

這個手法通常只會用在由 `高權限` 跳往 `低權限` 想要反過來的話則是靠另外的 Exception handling 機制來實現。

### Exception handling

觸發 Exception 有硬體觸發也有軟體觸發，為了實現主動提高權限的機制，這個部份就是要實現軟體觸發。  
無論是硬體或者是軟體觸發，當 Exception 發生之後要有一個對應的處理機制，  
處理流程大致如下:

1. 呼叫 `svc` 指令觸發 exception
2. 從 vector table 中找到對應的 exception handler 處理 exception 。

因為 vector table 每一個 entry 都有 128 bytes 的大小限制，
這代表能呼叫的指令個數上限為 `128 / 4 = 32` 。所以 vector table 的 exception handler 通常除了很固定的動作之外都會另外呼叫一個 proxy 。

所以可以區分的角色如下:

- stub: vector table 的 entry
- handler: 實際用來處理 exception 邏輯的 function

在處理 exception 的標準流程如下:

1. 將 exception flag mask 避免 exception 無限堆疊
2. 將 register set 狀態存起來
3. 呼叫 proxy 處理
4. 將 register set 狀態讀出來恢復狀態
5. 將 exception flag unmask
6. eret

在我的實做之中 handler 是指 proxy 會由 C 語言實做。而 stub 則是用 assembly code 來實做。

## Basic Exercise 2 - Interrupt



## Basic Exercise 3 - Rpi3’s Peripheral Interrupt


## Advanced Exercise 1 - Timer Multiplexing


## Advanced Exercise 2 - Concurrent I/O Devices Handling

