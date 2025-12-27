#include "exception.h"
#include "common.h"
#include "mini_uart.h"

// from current EL while using SP_ELx
void handler_type05(void);  // irq

// from lower EL and at least one lower EL is AArch64
void handler_type08(void);  // synchronous

void handler_type05(void) {

}

void handler_type08(void) {
    uint32_t esr, ec;
    
    asm volatile("mrs %0, esr_el1": "=r"(esr));
    ec = esr >> 26;

    switch (ec) {
    case 0x15:  // SVC instruction execution
        mini_uart_putln("Trapped SVC instruction executed in AArch64 state.");
        break;
    default:
        mini_uart_puts("Unknown exception type8: esr_el1 = ");
        mini_uart_putln(uint64_to_hex_str(esr, 8, NULL));
        break;
    }
}