#include "shell.h"
#include "config.h"
#include "printf.h"
#include "mini_uart.h"
#include "string.h"
#include "mailbox.h"
#include "power.h"
#include "cpio.h"
#include "timer.h"
#include "memory.h"
#include "frame.h"
// #include "sched.h"
#include "util.h"

static char buffer[SHELL_BUFFER_MAX_SIZE] = {0};
static int len;
static char tokens[SHELL_TOKEN_MAX_NUM][SHELL_TOKEN_MAX_LEN] = {0};
static int token_num = 0;
static const char *err_msg[] = {
    "Reading error: command is too long.",
    "Parsing error: too many tokens.",
    "Parsing error: token length is too long.",
    "Empty command.",
};

static int read_command(void);
static int parse_command(void);
static void execute_command(void);

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
static void command_memory_layout(void);
// static void command_thread_demo(void);

// for command_mailbox
void get_board_revision(void);
void get_arm_memory_info(void);

// for "timer" command
void shell_timer_event_cb(void *arg);

// for command_thread_demo
// void demo_task(void *arg);

void shell(void){
    int err_code = 0;
    printf("Welcome to the OGC shell. Use \"help\" for information on supported commands.\n");
    while(true){
        new_line();
        err_code = read_command();
        if(err_code){
            printf("\n%s\n", err_msg[err_code - 1]);
            continue;
        }
        err_code = parse_command();
        if(err_code){
            printf("\n%s\n", err_msg[err_code - 1]);
            continue;
        }

        execute_command();
    }
}

void new_line(void){
    printf("\r$ ");
    len = 0;
    token_num = 0;
}

static int read_command(void){
    len = 0;
    char c = '\0';
    do{
        c = uart_getc();
        
        if(c == '\n'){
            buffer[len++] = '\0';
            printf("\n");
        }else if(c == 0x7F){    // c = del
            if(len){
                printf("%c %c", (char)0x8, (char)0x8);  // print 2 backspaces to remove del and one char
                buffer[len--] = '\0';
            }
        }else if(c > 0x1F){
            buffer[len++] = c;
            printf("%c", c);
        }
    }while(c != '\n' && len < SHELL_BUFFER_MAX_SIZE);
    if(len == SHELL_BUFFER_MAX_SIZE && buffer[SHELL_BUFFER_MAX_SIZE - 1] != '\0'){
        return 1;
    }

    return 0;
}

static int parse_command(void){
    char *c = buffer;
    char *t;

    while(*c != '\0'){
        if(token_num == SHELL_TOKEN_MAX_NUM){
            return 2;
        }

        while(*c == ' '){
            c++;
        }
        
        t = tokens[token_num];
        while(*c != ' ' && *c != '\0'){
            if(t == tokens[token_num] + SHELL_TOKEN_MAX_LEN - 1){
                return 3;
            }
            *(t++) = *(c++);
        }
        *t = '\0';

        if(t != tokens[token_num]){
            token_num++;
        }
    }
    return token_num ? 0 : 4;
}

static void execute_command(void){
    if(!strncmp(tokens[0], "help", SHELL_TOKEN_MAX_LEN)){
        command_help();
    }else if(!strncmp(tokens[0], "hello", SHELL_TOKEN_MAX_LEN)){
        command_hello();
    }else if(!strncmp(tokens[0], "mailbox", SHELL_TOKEN_MAX_LEN)){
        command_mailbox();
    }else if(!strncmp(tokens[0], "reboot", SHELL_TOKEN_MAX_LEN)){
        command_reboot();
    }else if(!strncmp(tokens[0], "ls", SHELL_TOKEN_MAX_LEN)){
        command_ls();
    }else if(!strncmp(tokens[0], "cat", SHELL_TOKEN_MAX_LEN)){
        command_cat();
    }else if(!strncmp(tokens[0], "exec", SHELL_TOKEN_MAX_LEN)){
        command_exec();
    }else if(!strncmp(tokens[0], "timer", SHELL_TOKEN_MAX_LEN)){
        command_timer();
    }else if(!strncmp(tokens[0], "malloc", SHELL_TOKEN_MAX_LEN)){
        command_malloc();
    }else if(!strncmp(tokens[0], "free", SHELL_TOKEN_MAX_LEN)){
        command_free();
    }else if(!strncmp(tokens[0], "memory_layout", SHELL_TOKEN_MAX_LEN)){
        command_memory_layout();
    // }else if(!strncmp(tokens[0], "thread_demo", SHELL_TOKEN_MAX_LEN)){
    //     command_thread_demo();
    }else{
        printf("Unsupported command: %s\n""\n", buffer);
    }
}

static void command_help(void){
    printf(
        "The basic format of a command is \"{Command} [arg1 arg2 ...]\".\n"
        "Argument is a token string in the format \"[arg]\", \"[-arg]\" or \"[-arg=xxx]\".\n"
        "Each token has a maximum length of 32 characters (including the null byte).\n"
        "A command with the suffix \"*\" indicates that it has arguments.\n"
        "Use the argument \"-h\" or \"-help\" to find more details.\n"
        "\n"
        "help\t\t: Display the help menu.\n"
        "hello\t\t: Print \"Hello World!\"\n"
        "mailbox*\t: Communicate with the VideoCoreIV GPU.\n"
        "reboot\t\t: Reboot system.\n"
        "ls*\t\t: List the file names in ramdisk.\n"
        "cat*\t\t: Display the file content in ramdisk.\n"
        "exec*\t\t: Execute a program in Ramdisk.\n"
        "timer*\t\t: Display the number of seconds since booting and trigger a delayed timer interrupt.\n"
        "malloc*\t\t: Allocates and returns a pointer to the allocated memory.\n"
        "free*\t\t: Deallocated memory.\n"
        "memory_layout\t: Display the current layout of memory system.\n"
        // "thread_demo\t: Demo thread creation.\n"
        "\n"
    );
}

static void command_hello(void){
    printf("Hello world!\n""\n");
}

static void command_mailbox(void){
    is_help(
        printf(
            "<default>\t\t: with all optional arguments.\n"
            "[-revision]\t\t: Display the board revision.\n"
            "[-arm-memory-info]\t: Display the ARM memory base address and size.\n"
            "\n"
        );
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
                printf("Unsupported argument: %s\n", tokens[i]);
            }
        }
    }
    printf("\n");
}

static void command_reboot(void){
    printf("\n");
    power_reset(100);
}

static void command_ls(void){
    int is_match;
    void *current;
    file_info_t info;

    is_help(
        printf(
            "<default>\t: Display all file names in the ramdisk.\n"
            "{file name}\t: Check whether the file is in the ramdisk. You can specify multiple files at once.\n"
            "\n"
        );
    );

    if(token_num == 1){
        current = get_cpio_base();
        while(current != 0){
            // extract file info
            if(cpio_file_iter(&current, &info)){ // abnormal iter 
                break;
            }
            printf("%s\n", info.name);
        }
    }else{
        for(int i = 1; i < token_num; i++){
            is_match = 0;
            current = get_cpio_base();
            while(current != 0 && !is_match){
                // extract file info
                if(cpio_file_iter(&current, &info)){ // abnormal iter 
                    break;
                }
                is_match = !strcmp(tokens[i], info.name);
            }
            if(is_match){
                printf("File \"%s\" exists.\n", tokens[i]);
            }else{
                printf("File \"%s\" does not exist.\n", tokens[i]);
            }
        }
    }
    printf("\n");
}

static void command_cat(void){
    void *current;
    file_info_t info;

    is_help(
        printf("{file name}: Display the file content. You can specify multiple files at once.\n""\n");
    );

    if(token_num == 1){
        printf("You must provide at least one file name.\n""\n");
    }else{
        for(int i = 1; i < token_num; i++){
            current = get_cpio_base();
            while(current != 0){
                // extract file info
                if(cpio_file_iter(&current, &info)){ // abnormal iter 
                    break;
                }

                if(!strcmp(tokens[i], info.name)){
                    char *s = (char*)info.content;
                    for(uint32_t i = 0; i < info.content_size; i++){
                        printf("%c", s[i]);
                    }
                    printf("\n");
                    break;
                }
            }
            printf("\n");
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
        printf("{file name}\t: The file name of the program. You can specify exactly one file at a time.\n");
    );

    if(token_num < 2){
        printf("Program file name is required.\n");
    }else if(token_num > 2){
        printf("You cannot assign more than one program at a time.\n");
    }else{
        current = get_cpio_base();
        while(current != NULL){
            // extract file info
            if(cpio_file_iter(&current, &info)){ // abnormal iter 
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

        printf("Program \"%s\" does not exist.\n", tokens[1]);
    }
}

static void command_timer(void){
    uint64_t countdown = 2;
    void *arg = NULL;
    
    is_help(
        printf(
            "<default>\t: countdown = 2 and message = \"<Timer>: The number of seconds since booting is {sys_time}\".\n"
            "[-countdown=?]\t: The countdown (in decimal format, seconds) set for the timer.\n"
            "[-message=?]\t: The message that will be prompted after the duration expires.\n"
            "\n"
        );
    );

    for(int i = 1; i < token_num; i++){
        if(!strncmp(tokens[i], "-countdown=", 11)){
            countdown = dec_str_to_uint(tokens[i] + 11, -1);
        }else if(!strncmp(tokens[i], "-message=", 9)){
            arg = malloc(str_len(tokens[i] + 9) + 1);
            strcpy(tokens[i] + 9, (char*)arg);
        }else{
            printf("Unsupport argument: \"%s\"\n""\n", tokens[i]);
            return;
        }
    }

    uint64_t time = timer_get_current_time(SECOND);
    printf("The number of seconds since booting is %d.\n""\n", time);
    timer_add_timeout_event(SECOND, countdown, 0, shell_timer_event_cb, arg);
}

static void command_malloc(void){
    is_help(
        printf("{size}\t: The size of the memory in bytes that will be allocated.\n""\n");
    );
    
    if(token_num < 2){
        printf("\"size\" is required.\n""\n");
        return;
    }

    uint64_t size = dec_str_to_uint(tokens[1], -1);

    void *ptr = malloc(size);
    
    printf("malloc size %s bytes at 0x%s.\n""\n", tokens[1], uint_to_hex_str((uint64_t)ptr, 8, NULL));
}

static void command_free(void){
    is_help(
        printf("{address}\t: The memory address in hex format that will be deallocated.\n""\n");
    );

    if(token_num < 2){
        printf("\"address\" is required.\n""\n");
        return;
    }

    if(strncmp(tokens[1], "0x", 2)){
        printf("address should be in hex format.\n""\n");
        return;
    }

    void *addr = (void*)hex_str_to_uint(tokens[1] + 2, -1);
    free(addr);

    printf("free memory at %s\n""\n", tokens[1]);
}

static void command_memory_layout(void){
    buddy_sys_show_layout();
    printf("\n");
}

// #define DEMO_THREAD_NUM 3
// #define DEMO_LOOP_NUM   10
// static void command_thread_demo(void){
//     for(int i = 0; i < DEMO_THREAD_NUM; i++)
//         thread_create(demo_task, NULL);
    
// }

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
        printf("Board revision\t: 0x%s\n", uint_to_hex_str(mbox[5], 0, NULL));
    }else{
        printf("Mailbox error: get_board_revision()");
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
        printf("ARM memory base\t: 0x%s\n", uint_to_hex_str(mbox[5], 0, NULL));
        printf("ARM memory size\t: 0x%s\n", uint_to_hex_str(mbox[6], 0, NULL));
    }else{
        printf("Mailbox error: get_arm_memory_info()");
    }
}

// void demo_task(void *arg){
//     for(int i = 0; i < DEMO_LOOP_NUM; i++){
//         printf("\rThread id: %d, i = %d\n$ ", get_current_task()->pid, i);
//         wait_cycles(1000000);
//         schedule();
//     }
// }

void shell_timer_event_cb(void *arg){
    printf("\r");
    if(arg == NULL){
        printf("<Timer>: The number of seconds since booting is %d.\n""\n", timer_get_current_time(SECOND));
    }else{
        char *msg = (char*)arg;
        printf("%s\n""\n", msg);
        free(arg);
    }
    new_line();
}