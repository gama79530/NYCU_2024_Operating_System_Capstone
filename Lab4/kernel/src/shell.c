#include "shell.h"
#include "mini_uart.h"
#include "string.h"
#include "mailbox.h"
#include "power.h"
#include "cpio.h"
#include "timer.h"
#include "memory.h"

static char buffer[BUFFER_MAX_SIZE] = {0};
static char tokens[TOKEN_NUM_MAX][TOKEN_MAX_LEN] = {0};
static int token_num = 0;
static uint32_t err_code = 0;

static void read_command(void);
static void parse_command(void);
static void execute_command(void);
static void parse_error(void);

#define is_help(cmds) do{\
    for(int i = 1; i < token_num; i++){\
        if(!strcmp(tokens[i], "-h") || !strcmp(tokens[i], "-help")){\
            cmds\
            return;\
        }\
    }\
}while(0)

static void command_help(void);
static void command_hello(void);
static void command_mailbox(void);
static void command_reboot(void);
static void command_ls(void);
static void command_cat(void);
static void command_exec(void);
static void command_timer(void);
static void command_malloc(void);
static void command_free(void);

// for command_mailbox
void get_board_revision(void);
void get_arm_memory_info(void);

void shell(void){
    uart_putln("Welcome to the OGC shell. Use \"help\" for information on supported commands.");

    while(1){
        uart_puts("$ ");
        read_command();
        parse_command();
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
    }while(c != '\n' && index < BUFFER_MAX_SIZE - 1);
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
        }else if(!strncmp(tokens[0], "exec", TOKEN_MAX_LEN)){
            command_exec();
        }else if(!strncmp(tokens[0], "timer", TOKEN_MAX_LEN)){
            command_timer();
        }else if(!strncmp(tokens[0], "malloc", TOKEN_MAX_LEN)){
            command_malloc();
        }else if(!strncmp(tokens[0], "free", TOKEN_MAX_LEN)){
            command_free();
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
    uart_putln("The basic format of a command is \"{Command} [arg1 arg2 ...]\".");
    uart_putln("Argument is a token string in the format \"[arg]\", \"[-arg]\" or \"[-arg=xxx]\".");
    uart_putln("Each token has a maximum length of 32 characters (including the null byte).");
    uart_putln("A command with the suffix \"*\" indicates that it has arguments.");
    uart_putln("Use the argument \"-h\" or \"-help\" to find more details.");
    uart_putln("");
    uart_putln("help\t: Display the help menu.");
    uart_putln("hello\t: Print \"Hello World!\"");
    uart_putln("mailbox*: Communicate with the VideoCoreIV GPU.");
    uart_putln("reboot\t: Reboot system");
    uart_putln("ls*\t: List the file names in ramdisk.");
    uart_putln("cat*\t: Display the file content in ramdisk.");
    uart_putln("exec*\t: Execute a program in Ramdisk.");
    uart_putln("timer*\t: Display the number of seconds since booting and trigger a delayed timer interrupt.");
    uart_putln("malloc*\t: Allocates and returns a pointer to the allocated memory");
    uart_putln("free*\t: Deallocated memory.");
}

static void command_hello(void){
    uart_putln("Hello world!");
}

static void command_mailbox(void){
    is_help(
        uart_putln("<default>\t\t: with all optional arguments.");
        uart_putln("[-revision]\t\t: Display the board revision.");
        uart_putln("[-arm-memory-info]\t: Display the ARM memory base address and size.");
    );


    if(token_num == 1){
        get_board_revision();
        get_arm_memory_info();
    }else{
        for(int i = 1; i < token_num; i++){
            if(!strcmp(tokens[i], "-revision")){
                get_board_revision();
            }else if(!strcmp(tokens[i], "-arm-memory-info")){
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
    int is_match;
    void *current;
    file_info_t info;

    is_help(
        uart_putln("<default>\t: Display all file names in the ramdisk.");
        uart_putln("{file name}\t: Check whether the file is in the ramdisk. You can specify multiple files at once.");
    );

    if(token_num == 1){
        current = get_cpio_begin_ptr();
        while(current != 0){
            // extract file info
            if(cpio_iter(&current, &info)){ // abnormal iter 
                break;
            }
            uart_putln(info.name);
        }
    }else{
        for(int i = 1; i < token_num; i++){
            is_match = 0;
            current = get_cpio_begin_ptr();
            while(current != 0 && !is_match){
                // extract file info
                if(cpio_iter(&current, &info)){ // abnormal iter 
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
    void *current;
    file_info_t info;

    is_help(
        uart_putln("{file name}: Display the file content. You can specify multiple files at once.");
    );

    if(token_num == 1){
        uart_putln("You must provide at least one file name.");
    }else{
        for(int i = 1; i < token_num; i++){
            current = get_cpio_begin_ptr();
            while(current != 0){
                // extract file info
                if(cpio_iter(&current, &info)){ // abnormal iter 
                    break;
                }

                if(!strcmp(tokens[i], info.name)){
                    uart_put_mutiln(info.content, info.content_size);
                    break;
                }
            }
            uart_putln("");
        }
    }
}

static void command_exec(void){
    void *current;
    file_info_t info;
    char *user_prog;
    const uint64_t spsr_el1 = 0x340; // DAF masked + EL0t
    const uint64_t load_addr = 0x20000;
    const uint64_t stack_ptr = load_addr + 0x2000;

    is_help(
        uart_putln("{file name}\t: The file name of the program. You can specify exactly one file at a time.");
    );

    if(token_num < 2){
        uart_putln("Program file name is required.");
    }else if(token_num > 2){
        uart_putln("You cannot assign more than one program at a time.");
    }else{
        current = get_cpio_begin_ptr();
        while(current != NULL){
            // extract file info
            if(cpio_iter(&current, &info)){ // abnormal iter 
                break;
            }

            if(!strcmp(tokens[1], info.name)){
                // copy file to user program load address
                user_prog = (char*)load_addr;
                for(int i = 0; i < info.content_size; i++){
                    user_prog[i] = ((char*)info.content)[i];
                }
                
                // Switch to EL0 and execute user program
                asm volatile("msr spsr_el1, %0" : : "r"(spsr_el1));
                asm volatile("msr elr_el1, %0" : : "r"(load_addr));
                asm volatile("msr sp_el0, %0" : : "r"(stack_ptr));
                asm volatile("eret");
            }
        }

        uart_puts("Program \"");
        uart_puts(tokens[1]);
        uart_putln("\" does not exist.");
    }
}

static void command_timer(void){
    uint64_t duration = 2;
    const char *msg = NULL;
    
    is_help(
        uart_putln("<default>\t: duration = 2 and message = \"<Timer>: The number of seconds since booting is {sys_time}\".");
        uart_putln("[-duration=?]\t: The duration (in decimal format, seconds) set for the timer.");
        uart_putln("[-message=?]\t: The message that will be prompted after the duration expires.");
    );

    for(int i = 1; i < token_num; i++){
        if(!strncmp(tokens[i], "-duration=", 10)){
            duration = dec_str_to_uint(tokens[i] + 10);
        }else if(!strncmp(tokens[i], "-message=", 9)){
            msg = tokens[i] + 9;
        }else{
            uart_puts("Unsupport argument: ");
            uart_putln(tokens[i]);
            return;
        }
    }

    uint64_t time = timer_get_current_time();
    uart_puts("The number of seconds since booting is ");
    uart_puts(uint_to_dec_str(time));
    uart_putln(".");
    timer_add_timeout_event(duration, msg);
}

static void command_malloc(void){
    is_help(
        uart_putln("{size}\t: The size of the memory in bytes that will be allocated.");
    );
    
    if(token_num < 2){
        uart_putln("\"size\" is required.");
        return;
    }

    uint64_t size = dec_str_to_uint(tokens[1]);

    void *ptr = malloc(size);
    
    uart_puts("malloc size ");
    uart_puts(tokens[1]);
    uart_puts(" bytes at 0x");
    uart_putln(long_to_hex_str((uint64_t)ptr) + 8);
}

static void command_free(void){
    is_help(
        uart_putln("{address}\t: The memory address in hex format that will be deallocated.");
    );

    if(token_num < 2){
        uart_putln("\"address\" is required.");
        return;
    }

    if(strncmp(tokens[1], "0x", 2)){
        uart_putln("address should be in hex format.");
        return;
    }

    void *addr = (void*)hex_str_to_uint(tokens[1] + 2);
    free(addr);

    uart_puts("free memory at ");
    uart_putln(tokens[1]);
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
