#include "mini_uart.h"
#include "shell.h"

void bootloader_main(uint64_t dtb_addr)
{
    mini_uart_init();
    shell_run(dtb_addr);
}
