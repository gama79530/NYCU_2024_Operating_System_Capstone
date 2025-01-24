#include "exception.h"
#include "list.h"
#include "util.h"
#include "peripheral.h"
#include "memory.h"
#include "mini_uart.h"
#include "config.h"
#include "timer.h"
#include "arm_v8.h"
#include "string.h"

typedef struct irq_task{
    list_head_t             anchor;
    irq_task_cb_t           callback;
    uint8_t                 priority;   // smaller number => higher priority
} irq_task_t;

static LIST_HEAD(blank_irq_tasks_q);
static LIST_HEAD(waiting_irq_tasks_q);

void show_invalid_entry_message(uint64_t type, uint64_t spsr_el1, uint64_t esr_el1, uint64_t elr_el1);
void irq_el1h(void);
void irq_el0_64(void);

void irq_handler(void);
irq_task_t* get_blank_irq_task(void);
void add_waiting_irq_task(irq_task_t *task);
void execute_waiting_irq_task(void);

const char* entry_error_type[] = {
    [SYNC_INVALID_EL1t]     = "SYNC_INVALID_EL1t",
    [IRQ_INVALID_EL1t]      = "IRQ_INVALID_EL1t",
    [FIQ_INVALID_EL1t]      = "FIQ_INVALID_EL1t",
    [ERROR_INVALID_EL1t]    = "ERROR_INVALID_EL1t",
    [SYNC_INVALID_EL1h]     = "SYNC_INVALID_EL1h",
    [IRQ_INVALID_EL1h]      = "IRQ_INVALID_EL1h",
    [FIQ_INVALID_EL1h]      = "FIQ_INVALID_EL1h",
    [ERROR_INVALID_EL1h]    = "ERROR_INVALID_EL1h",
    [SYNC_INVALID_EL0_64]   = "SYNC_INVALID_EL0_64",
    [IRQ_INVALID_EL0_64]    = "IRQ_INVALID_EL0_64",
    [FIQ_INVALID_EL0_64]    = "FIQ_INVALID_EL0_64",
    [ERROR_INVALID_EL0_64]  = "ERROR_INVALID_EL0_64",
    [SYNC_INVALID_EL0_32]   = "SYNC_INVALID_EL0_32",
    [IRQ_INVALID_EL0_32]    = "IRQ_INVALID_EL0_32",
    [FIQ_INVALID_EL0_32]    = "FIQ_INVALID_EL0_32",
    [ERROR_INVALID_EL0_32]  = "ERROR_INVALID_EL0_32",
    [SYNC_ERROR]            = "SYNC_ERROR",
    [SYSCALL_ERROR]         = "SYSCALL_ERROR",
};

void show_invalid_entry_message(uint64_t type, uint64_t spsr, uint64_t esr, uint64_t elr){
    disable_all_exception();

    uart_poll_puts(entry_error_type[type]);
    uart_poll_puts(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch (esr >> ESR_ELx_EC_SHIFT) {
    case 0b000000:
        uart_poll_puts("Unknown");
        break;
    case 0b000001:
        uart_poll_puts("Trapped WFI/WFE");
        break;
    case 0b001110:
        uart_poll_puts("Illegal execution");
        break;
    case 0b010101:
        uart_poll_puts("System call");
        break;
    case 0b100000:
        uart_poll_puts("Instruction abort, lower EL");
        break;
    case 0b100001:
        uart_poll_puts("Instruction abort, same EL");
        break;
    case 0b100010:
        uart_poll_puts("Instruction alignment fault");
        break;
    case 0b100100:
        uart_poll_puts("Data abort, lower EL");
        break;
    case 0b100101:
        uart_poll_puts("Data abort, same EL");
        break;
    case 0b100110:
        uart_poll_puts("Stack alignment fault");
        break;
    case 0b101100:
        uart_poll_puts("Floating point");
        break;
    default:
        uart_poll_puts("Unknown");
        break;
    }
    // decode data abort cause
    if (esr >> ESR_ELx_EC_SHIFT == 0b100100 ||
        esr >> ESR_ELx_EC_SHIFT == 0b100101) {
        uart_poll_puts(", ");
        switch ((esr >> 2) & 0x3) {
        case 0:
            uart_poll_puts("Address size fault");
            break;
        case 1:
            uart_poll_puts("Translation fault");
            break;
        case 2:
            uart_poll_puts("Access flag fault");
            break;
        case 3:
            uart_poll_puts("Permission fault");
            break;
        }
        switch (esr & 0x3) {
        case 0:
            uart_poll_puts(" at level 0");
            break;
        case 1:
            uart_poll_puts(" at level 1");
            break;
        case 2:
            uart_poll_puts(" at level 2");
            break;
        case 3:
            uart_poll_puts(" at level 3");
            break;
        }
    }

    // dump registers
    char buffer[9];
    uart_poll_puts(":\n, SPSR: 0x");
    uart_poll_puts(uint_to_hex_str(spsr, 0, buffer));
    uart_poll_puts(", ESR: 0x");
    uart_poll_puts(uint_to_hex_str(esr, 0, buffer));
    uart_poll_puts(", ELR: 0x");
    uart_poll_puts(uint_to_hex_str(elr, 0, buffer));
    uart_poll_puts("\n");

    enable_all_exception();
}

void irq_el1h(void){
    irq_handler();
}

void irq_el0_64(void){
    irq_handler();
}

void irq_handler(void){
    irq_task_t *task = NULL;

    disable_all_exception();
    if(get32(IRQ_PENDING_1) & IRQ_AUX_INT){         // UART IRQ
        if(get32(AUX_MU_IIR) & AUX_IRQ_RX){         // Receiver holds valid byte
            uart_rx_clr();
            task = get_blank_irq_task();
            task->priority = UART_IRQ_PRIORITY;
            task->callback = uart_irq_task_cb_rx;
        }else if(get32(AUX_MU_IIR) & AUX_IRQ_TX){   // Transmit holding register empty
            uart_tx_clr();
            task = get_blank_irq_task();
            task->priority = UART_IRQ_PRIORITY;
            task->callback = uart_irq_task_cb_tx;
        }
    }else if(get32(CORE0_IRQ_SOURCE) & CNTPNSIRQ){  // timer IRQ
        disable_core0_timer();
        task = get_blank_irq_task();
        task->priority = TIMER_IRQ_PRIORITY;
        task->callback = irq_timer_event;
    }else{
        uart_poll_putln("Unknown pending interrupt");
        uart_poll_putln("");
    }
    add_waiting_irq_task(task);
    execute_waiting_irq_task();
    enable_all_exception();
}

irq_task_t* get_blank_irq_task(void){
    irq_task_t *task = NULL;
    if(list_is_empty(&blank_irq_tasks_q)){
        task = (irq_task_t*)startup_alloc(sizeof(irq_task_t));
        if(task == NULL)
            task = (irq_task_t*)malloc(sizeof(irq_task_t));
    }else{
        task = container_of(blank_irq_tasks_q.next, irq_task_t, anchor);
        list_remove(&task->anchor);
    }

    if(task != NULL){
        task->priority = 255;
        task->callback = NULL;
    }
    
    return task;
}

void add_waiting_irq_task(irq_task_t *task){
    if(task){
        list_head_t *prev = &waiting_irq_tasks_q;
        list_head_t *next = prev->next;

        while(!list_is_head_node(next, &waiting_irq_tasks_q) && container_of(next, irq_task_t, anchor)->priority <= task->priority){
            prev = next;
            next = next->next;
        }

        list_add(&task->anchor, prev, next);
    }
}

void execute_waiting_irq_task(void){
    irq_task_t *task = NULL;
    while(!list_is_empty(&waiting_irq_tasks_q)){
        task = container_of(waiting_irq_tasks_q.next, irq_task_t, anchor);
        list_remove(&task->anchor);
        enable_all_exception();
        task->callback();
        disable_all_exception();
        
        list_add_last(&task->anchor, &blank_irq_tasks_q);
    }
}
