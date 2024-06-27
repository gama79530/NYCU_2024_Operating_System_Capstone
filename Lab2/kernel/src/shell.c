#include "shell.h"
#include "mini_uart.h"
#include "string.h"
#include "mailbox.h"
#include "power.h"
#include "cpio.h"

#define KERNEL_ADDR 0x80000

extern unsigned int kernel_size;
extern unsigned long dtb_ptr;

static char buffer[BUFFER_MAX_SIZE] = {0};
static char tokens[TOKEN_NUM_MAX][TOKEN_MAX_LEN] = {0};
static int token_num = 0;
static unsigned int err_code = 0;

static void read_command(void);
static void parse_command(void);
static void execute_command(void);

static void parse_error(void);
static void command_help(void);
static void command_hello(void);
static void command_mailbox(void);
static void command_reboot(void);
static void command_ls(void);
static void command_cat(void);

void get_board_revision(void);
void get_arm_memory_info(void);


static void read_command(void){
    for(char *c = buffer; c != buffer + BUFFER_MAX_SIZE - 1; c++){
        *c = uart_getc();
        if(*c == '\n'){
            *c = '\0';
            break;
        }else{
            uart_putc(*c);   
        }
    }
    uart_puts("\r\n");
}

static void parse_command(void){
    char *c = buffer;
    char *t;
    err_code = 0;
    token_num = 0;

    while(!err_code && *c != '\0'){
        if(token_num == TOKEN_NUM_MAX){
            err_code = 1;
            break;
        }

        while(*c == ' '){
            c++;
            continue;
        }
        
        t = tokens[token_num];
        while(*c != ' ' && *c != '\0'){
            if(t == tokens[token_num] + TOKEN_MAX_LEN - 1){
                err_code = 2;
                break;
            }

            *t = *c;
            t++;
            c++;
        }
        *t = '\0';

        if(t != tokens[token_num]){
            token_num++;
        }
    }
}

static void execute_command(void){
    if(err_code){
        parse_error();
    }else if(!token_num){
        uart_putln("Empty command.");
    }else{
        if(!strncmp(tokens[0], "help", TOKEN_MAX_LEN)){
            command_help();
        }else if(!strncmp(tokens[0], "hello", TOKEN_MAX_LEN)){
            command_hello();
        }else if(!strncmp(tokens[0], "mailbox", TOKEN_MAX_LEN)){
            command_mailbox();
        }else if(!strncmp(tokens[0], "reboot", TOKEN_MAX_LEN)){
            command_reboot();
        }else if(!strncmp(tokens[0], "ls", TOKEN_MAX_LEN)){
            command_ls();
        }else if(!strncmp(tokens[0], "cat", TOKEN_MAX_LEN)){
            command_cat();
        }else{
            uart_puts("Unsupported command: ");
            uart_putln(buffer);
        }
    }
}

static void parse_error(void){
    if(err_code == 1){
        uart_putln("Parsing error: too many tokens.");
    }else if(err_code == 2){
        uart_putln("Parsing error: token length is too long.");
    }
}

static void command_help(void){
    uart_putln("The basic format of a command is \"{Command} [options...]\". Each token has a maximum length of 31 characters.");
    uart_putln("A command with the suffix \"*\" indicates that it has optional arguments.");
    uart_putln("You can use \"{Command} --help\" to find more details.");
    uart_putln("");
    uart_putln("help\t: Display the help menu.");
    uart_putln("hello\t: Print \"Hello World!\"");
    uart_putln("mailbox*: Communicate with the VideoCoreIV GPU.");
    uart_putln("reboot\t: Reboot system");
    uart_putln("ls*\t: List all file names in ramdisk.");
    uart_putln("cat*\t: Display file name and content.");
}

static void command_hello(void){
    uart_putln("Hello world!");
}

static void command_mailbox(void){
    if(token_num == 1){
        get_board_revision();
        get_arm_memory_info();
    }else{
        for(int i = 1; i < token_num; i++){
            if(!strcmp(tokens[i], "--help")){
                uart_putln("[default]\t\t: Display all the following information.");
                uart_putln("--revision\t\t: Display the board revision.");
                uart_putln("--arm-memory-info\t: Display the ARM memory base address and size.");
            }else if(!strcmp(tokens[i], "--revision")){
                get_board_revision();
            }else if(!strcmp(tokens[i], "--arm-memory-info")){
                get_arm_memory_info();
            }else{
                uart_puts("Unsupported argument: ");
                uart_putln(tokens[i]);
            }
        }
    }
}

static void command_reboot(void){
    power_reset(100);
}

static void command_ls(void){
    int is_help = 0;
    int is_match;
    char *current;
    file_info_t info;

    for(int i = 1; i < token_num && !is_help; i++){
        is_help = !strcmp(tokens[i], "--help");
    }

    if(is_help){
        uart_putln("[default]\t: Display all file names in the ramdisk.");
        uart_putln("{file name}\t: Check whether the file is in the ramdisk.");
        uart_putln("\t\t  You can specify multiple files at once.");
    }else if(token_num == 1){
        current = get_cpio_ptr();
        while(current != 0){
            // extract file info
            if(iter(&current, &info)){
                break;
            }
            uart_putln(info.name);
        }
    }else{
        for(int i = 1; i < token_num; i++){
            is_match = 0;
            current = get_cpio_ptr();
            while(current != 0 && !is_match){
                // extract file info
                if(iter(&current, &info)){
                    break;
                }
                is_match = !strcmp(tokens[i], info.name);
            }
            if(is_match){
                uart_puts("File \"");
                uart_puts(tokens[i]);
                uart_putln("\" exists.");
            }else{
                uart_puts("File \"");
                uart_puts(tokens[i]);
                uart_putln("\" does not exist.");
            }
        }
    }
}

static void command_cat(void){
    int is_help = 0;
    int is_root = 1;
    char *current;
    file_info_t info;

    for(int i = 1; i < token_num && !is_help; i++){
        is_help = !strcmp(tokens[i], "--help");
    }

    if(is_help){
        uart_putln("[default]\t: Display all file names and their contents in the ramdisk.");
        uart_putln("{file name}\t: Display the file name and its content if it is in the ramdisk.");
        uart_putln("\t\t  You can specify multiple files at once.");
    }else if(token_num == 1){
        current = get_cpio_ptr();
        while(current != 0){
            // extract file info
            if(iter(&current, &info)){
                break;
            }

            if(is_root){
                is_root = 0;
                continue;
            }
            
            uart_putln(info.name);
            uart_put_mutiln(info.content, info.content_size);
            uart_putln("");
        }
    }else{
        for(int i = 1; i < token_num; i++){
            current = get_cpio_ptr();
            while(current != 0){
                // extract file info
                if(iter(&current, &info)){
                    break;
                }

                if(!strcmp(tokens[i], info.name)){
                    uart_putln(info.name);
                    uart_put_mutiln(info.content, info.content_size);
                    break;
                }
            }
            uart_putln("");
        }
    }
}

void get_board_revision(void){
    mbox[0] = 7 * 4; // buffer size in bytes
    mbox[1] = MBOX_REQUEST;
    // tags begin
    mbox[2] = MBOX_TAG_GETREVISION; // tag identifier
    mbox[3] = 4; // maximum of request and response value buffer's length.
    mbox[4] = MBOX_TAG_REQUEST;
    mbox[5] = 0; // buffer for revision
    mbox[6] = MBOX_TAG_END;
    // tags end

    if(mailbox_call(MBOX_CH_PROP)){
        uart_puts("Board revision\t: 0x");
        uart_putln(int_to_hex_str(mbox[5]));
    }else{
        uart_putln("Mailbox error: get_board_revision()");
    }
}

void get_arm_memory_info(void){
    mbox[0] = 8 * 4;
    mbox[1] = MBOX_REQUEST;
    // tags begin
    mbox[2] = MBOX_TAG_GETMEMORY;
    mbox[3] = 8; // maximum of request and response value buffer's length.
    mbox[4] = MBOX_TAG_REQUEST;
    mbox[5] = 0; // buffer for memory base address
    mbox[6] = 0; // buffer for memory size
    mbox[7] = MBOX_TAG_END;
    // tags end

    if(mailbox_call(MBOX_CH_PROP)){
        uart_puts("ARM memory base\t: 0x");
        uart_putln(int_to_hex_str(mbox[5]));
        uart_puts("ARM memory size\t: 0x");
        uart_putln(int_to_hex_str(mbox[6]));
    }else{
        uart_putln("Mailbox error: get_arm_memory_info()");
    }
}

void shell(void){
    uart_putln("Welcome to the OGC shell. Use \"help\" for information on supported commands.");

    while(1){
        uart_puts("$ ");
        read_command();
        parse_command();
        execute_command();
    }
}

