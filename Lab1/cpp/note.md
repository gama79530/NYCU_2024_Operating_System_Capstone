# Note

## bare-metal, nostdlib 環境下的物件初始化問題

static/global 物件的 constructor 只會分配空間給物件但不會執行 `constructor`。

在一般作業系統中，編譯器與連結器會自動包含啟動碼（如 `crt0`、`crtbegin`、`crtend`），負責：
- 呼叫全域或 static 物件的 constructor，
- 呼叫 main()，
- 程式結束時呼叫 destructor。

然而，在 bare-metal 與 `-nostdlib` 環境下，這些啟動碼不存在，所以 static/global 物件只會被分配記憶體，但不會自動執行 constructor，導致物件內容可能是全 0 或未定值。

**解決方法：**
- **使用 stack 變數**：局部變數在進入函式時會自動初始化。  
- **使用 placement new**：手動在已分配的 buffer 上呼叫 constructor 來初始化物件。

例如：

```cpp
#include <new> // 提供 placement new 的宣告

alignas(Kernel) static char kernelBuffer[sizeof(Kernel)];
Kernel *kernel = new(kernelBuffer) Kernel();
```

## ndefined reference to memset 問題

### 原始錯誤訊息

```bash
aarch64-linux-gnu-ld: build/shell_cpp.o: in function Shell::Shell()': shell.cpp:(.text+0x24): undefined reference to memset'
aarch64-linux-gnu-ld: shell.cpp:(.text+0x4c): undefined reference to `memset'
```

### 原因

原本在 class 中有這樣的宣告：

```cpp
char buffer[SHELL_BUFFER_MAX_SIZE + 1] = {0};
char tokens[SHELL_TOKEN_MAX_NUM][SHELL_TOKEN_MAX_LEN + 1] = {0};
```

使用 `{0}` 初始化會產生依賴 memset 的代碼來自動清零記憶體。在裸機環境下，由於沒有標準庫提供 memset，所以連結失敗。

### 解決方法

改為：

```cpp
char buffer[SHELL_BUFFER_MAX_SIZE + 1];
char tokens[SHELL_TOKEN_MAX_NUM][SHELL_TOKEN_MAX_LEN + 1];
```
然後在 constructor 中手動初始化，例如用自定義的 memset 或直接撰寫迴圈初始化：
```cpp
for (int i = 0; i < SHELL_BUFFER_MAX_SIZE + 1; i++) {
    buffer[i] = 0;
}
```
或提供一個簡單的 memset 實作。