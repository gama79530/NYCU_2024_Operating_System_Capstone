#include "shell.h"
#include "mini_uart.h"
#include "string.h"
#include "mailbox.h"
#include "power.h"

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

        token_num++;
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
    uart_putln("help\t: display the help menu.");
    uart_putln("hello\t: print \"Hello World!\"");
    uart_putln("mailbox*: communicate with the VideoCoreIV GPU.");
    uart_putln("reboot\t: reboot system");
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
                uart_putln("[default]\t\t: display all the following information.");
                uart_putln("--revision\t\t: display the board revision.");
                uart_putln("--arm-memory-info\t: display the ARM memory base address and size.");
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

