#include "mini_uart.h"
#include "shell.h"

void main(){
    uart_init();
    uart_puts("\rWelcome to the OGC's shell. Using \"help\" for the information of supporting command.\r\n");

    while(1){
        accept_command();
    }
}