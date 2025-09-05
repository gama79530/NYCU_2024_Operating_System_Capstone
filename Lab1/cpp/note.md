# Note

## 在 bare-metal, nostdlib 環境下，static/global 物件的 constructor 不會自動執行


**只會分配空間給物件但不會執行 `constructor` **

### copilot 給的回答

標準 C++ 啟動流程
在一般作業系統或標準 C++ 程式中，
編譯器和連結器會加入一段「啟動碼」（startup code），
這段啟動碼（如 `crt0`, `crtbegin`, `crtend`）負責：

- 初始化全域/static 物件（呼叫 constructor）
- 執行 main() 函式
- 程式結束時呼叫 destructor

bare-metal, nostdlib 的情況
- `-nostdlib` 代表不使用標準 C/C++ 函式庫和啟動碼。
- 在裸機環境，通常只有你自己寫的啟動程式（如 bootloader），沒有自動呼叫 constructor/destructor 的機制。
- 所以 static/global 物件（例如 static Kernel kernel;）只會分配空間，不會自動執行建構子，
物件內容可能是全 0 或未定值。

## undefined reference to memset 

### 原始錯誤訊息

```
aarch64-linux-gnu-ld: build/shell_cpp.o: in function Shell::Shell()': shell.cpp:(.text+0x24): undefined reference to memset'
aarch64-linux-gnu-ld: shell.cpp:(.text+0x4c): undefined reference to `memset'
```

### 原因

原本我的 class 裡面有這一段

```cpp
char buffer[SHELL_BUFFER_MAX_SIZE + 1] = {0};
char tokens[SHELL_TOKEN_MAX_NUM][SHELL_TOKEN_MAX_LEN + 1] = {0};
```

這裡需要 `memset`，如果沒有的話就改成

```cpp
char buffer[SHELL_BUFFER_MAX_SIZE + 1];
char tokens[SHELL_TOKEN_MAX_NUM][SHELL_TOKEN_MAX_LEN + 1];
```

然後在 `constructor` 裡面處理初始化。

這個原因應該是因為這不是在 static 成員所以它會被放在 heap 或 stack ， 在這裡面的記憶體不會自動初始化。