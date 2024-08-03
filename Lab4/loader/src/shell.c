#include "shell.h"
#include "mini_uart.h"
#include "string.h"
#include "util.h"

#define KERNEL_ADDR 0x80000

extern uint64_t kernel_size;
extern uint64_t dtb_ptr;

static char buffer[BUFFER_MAX_SIZE] = {0};

static void read_command(void);
static void execute_command(void);

static void command_help(void);
static void command_download(void);
static void command_boot(void);
static void command_automatic(void);

void shell(void){
    uart_putln("Welcome to the OGC's loader. Use \"help\" for information on supported commands.\n");    
    while(1){
        uart_puts("$ ");
        read_command();
        execute_command();
    }
}

static void read_command(void){
    int index = 0;
    char c = 0;

    do{
        c = uart_getc();
        if(c == '\n'){
            buffer[index++] = '\0';
        }else if(c == 0x7F){
            if(index){
                uart_putc(0x8);
                uart_putc(' ');
                uart_putc(0x8);
                buffer[index--] = '\0';
            }
        }else if(c > 0x1F){
            buffer[index++] = c;
            uart_putc(c);
        }
    }while(index < BUFFER_MAX_SIZE - 1 && c != '\n');
    uart_puts("\r\n");
}

static void execute_command(void){
    if(!strcmp(buffer, "help")){
        command_help();
    }else if(!strcmp(buffer, "download")){
        command_download();
    }else if(!strcmp(buffer, "boot")){
        command_boot();
    }else if(!strcmp(buffer, "automatic")){
        command_automatic();
    }else{
        uart_putln("Unsupported command.");
    }
}

static void command_help(void){
    uart_putln("help\t\t: display the help menu.");
    uart_putln("download\t: download kernel image from PC client.");
    uart_putln("boot\t\t: jump to kernel image.");
    uart_putln("automatic\t: \"download\" + \"boot\"");
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
    for(uint64_t i = 0; i < kernel_size; i++){
        *kernel++ = uart_getb();
    }

    uart_puts("done$");
}

static void command_boot(void){
    if(kernel_size){
        uart_putln("boot kernel image...");
        wait_msec(1000);
        void (*kernel)(uint64_t) = (void (*)(uint64_t))KERNEL_ADDR;
        kernel(dtb_ptr);
    }else{
        uart_putln("The kernel image is not loaded.");
    }
}

static void command_automatic(void){
    command_download();
    command_boot();
}
