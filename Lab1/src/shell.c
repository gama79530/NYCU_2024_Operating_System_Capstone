#include "mini_uart.h"
#include "shell.h"
#include "string.h"
#include "mailbox.h"
#include "power.h"

static char buffer[BUFFER_MAX_SIZE] = {0};
static int err_code;
static char tokens[TOKEN_NUM_MAX][TOKEN_MAX_LEN] = {0};
static int token_num;

void read_command();
void parse_command();
void execute_command();
void command_help();
void command_hello();
void command_mailbox();
void get_board_revision();
void get_arm_memory_info();
void command_reboot();

void accept_command(){
    uart_puts("# ");
    read_command();
    uart_puts("\r\n");

    parse_command();

    execute_command();
    uart_puts("\r\n");
}

void read_command(){
    for(char *c = buffer; c != buffer + BUFFER_MAX_SIZE - 1; c++){
        *c = uart_getc();
        if(*c == '\n'){
            *c = '\0';
            break;
        }else{
            uart_putc((unsigned int)(*c));
        }
    }
}

void parse_command(){
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
                err_code = 1;
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

void execute_command(){    
    if(err_code){
        uart_puts("Parsing error: ");
        uart_puts(buffer);
        uart_puts("\r\n");
    }else if(token_num == 0){
        uart_puts("No command.\r\n");
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
            uart_puts("Unsupported command.\r\n");
        }
    }
    
}

void command_help(){
    uart_puts("The basic format of command is \"{Command} [options...]\". Each token has limited length 31.\r\n");
    uart_puts("Command with suffix \"*\" indicates that it has optional arguments.\r\n");
    uart_puts("You can use \"{Command} --help\" to find more details.\r\n");
    uart_puts("\r\n");
    uart_puts("help\t: display the help menu.\r\n");
    uart_puts("hello\t: print \"Hello World!\"\r\n");
    uart_puts("mailbox*: communicate with VideoCoreIV GPU.\r\n");
    uart_puts("reboot\t: reboot system\r\n");
}

void command_hello(){
    uart_puts("Hello world!\r\n");
}

void command_mailbox(){
    if(token_num == 1){
        get_board_revision();
        get_arm_memory_info();
    }else{
        for(int i = 1; i < token_num; i++){
            if(!strcmp(tokens[i], "--help")){
                uart_puts("[default]\t\t: display all the following information.\r\n");
                uart_puts("--revision\t\t: display the board revision.\r\n");
                uart_puts("--arm-memory-info\t: display the ARM memory base address and size.\r\n");
            }else if(!strcmp(tokens[i], "--revision")){
                get_board_revision();
            }else if(!strcmp(tokens[i], "--arm-memory-info")){
                get_arm_memory_info();
            }
        }
    }
}


void get_board_revision(){
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // buffer for revision
    // tags end
    mailbox[6] = END_TAG;

    if(mailbox_call(8)){
        uart_puts("Mailbox error: get_board_revision()\r\n");
    }else{
        uart_puts("Board Revision: 0x");
        uart_put_hex(mailbox[5]);
        uart_puts("\r\n");
    }
}

void get_arm_memory_info(){
    mailbox[0] = 8 * 4;
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEMORY_INFO;
    mailbox[3] = 8; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // buffer for memory base address
    mailbox[6] = 0; // buffer for memory size
    // tags end
    mailbox[7] = END_TAG;

    if(mailbox_call(8)){
        uart_puts("Mailbox error: get_arm_memory_info()\r\n");
    }else{
        uart_puts("ARM memory base: 0x");
        uart_put_hex(mailbox[5]);
        uart_puts("\r\n");
        uart_puts("ARM memory size: 0x");
        uart_put_hex(mailbox[6]);
        uart_puts("\r\n");
    }
}

void command_reboot(){
    reset();
}