#include "shell.h"
#include "mini_uart.h"
#include "string.h"

#define KERNEL_ADDR 0x80000

extern unsigned int kernel_size;
extern unsigned long dtb_ptr;

static char buffer[BUFFER_MAX_SIZE] = {0};

static void read_command(void);
static void execute_command(void);

static void command_help(void);
static void command_download(void);
static void command_boot(void);

static void read_command(void){
    for(char *c = buffer; c != buffer + BUFFER_MAX_SIZE - 1; c++){
        *c = uart_getc();
        if(*c == '\n'){
            *c = '\0';
            uart_puts("\r\n");
            break;
        }else{
            uart_putc(*c);   
        }
    }
}

static void execute_command(void){
    if(!strcmp(buffer, "help")){
        command_help();
    }else if(!strcmp(buffer, "download")){
        command_download();
    }else if(!strcmp(buffer, "boot")){
        command_boot();
    }else{
        uart_putln("Unsupported command.");
    }
}

static void command_help(void){
    uart_putln("help\t: display the help menu.");
    uart_putln("download: download kernel image from PC client.");
    uart_putln("boot\t: jump to kernel image.");
}

static void command_download(void){
    char *kernel = (char*)KERNEL_ADDR;
    
    // protocol 2: loader initiates the download process by sending a request string "$upload_kernel$\n" to uploader.
    uart_puts("$upload_kernel$\n");

    // protocol 4: loader receives the kernel size and replies with "start_upload$".
    kernel_size = uart_getb();
    kernel_size |= uart_getb() << 8;
    kernel_size |= uart_getb() << 16;
    kernel_size |= uart_getb() << 24;
    uart_puts("start_upload$");

    // protocol 6: loader receives the kernel image and replies with "done$".
    for(unsigned int i = 0; i < kernel_size; i++){
        *kernel++ = uart_getb();
    }

    uart_puts("done$");
}

static void command_boot(void){
    if(kernel_size){
        uart_putln("boot kernel image...");
        void (*kernel)(unsigned long) = (void (*)(unsigned long))KERNEL_ADDR;
        kernel(dtb_ptr);
    }else{
        uart_putln("The kernel image is not loaded.");
    }
}

void shell(void){
    uart_putln("Welcome to the OGC's loader. Use \"help\" for information on supported commands.\n");    
    while(1){
        uart_puts("$ ");
        read_command();
        execute_command();
    }
}

