#include "allocator.h"
#include "buddy.h"
#include "config.h"
#include "fdt.h"
#include "initramfs.h"
#include "daif.h"
#include "mini_uart.h"
#include "printf.h"
#include "shell.h"
#include "timer.h"
#include "types.h"

/* Private constants */

#define SPIN_TABLE_BEGIN 0x00000000UL
#define SPIN_TABLE_END   0x00001000UL

/* Private types */

/* Public function declarations */

/*
 * Kernel C entry point after early assembly boot.
 *
 * Initialize basic subsystems, discover the initramfs range from the DTB when
 * available, and then enter the interactive shell.
 */
void main(uint64_t dtb_addr);

/* Private function declarations */

/* Bridge the printf library's putc callback to mini UART output. */
static void printf_putc(void *context, char c);

/* Initialize the page allocator and reserve memory used during early boot. */
static buddy_error_t buddy_initialize(const fdt_t *fdt,
                                      uintptr_t initramfs_begin,
                                      uintptr_t initramfs_end);

/* Private data */

static fdt_t boot_fdt;

extern char kernel_begin;
extern char kernel_end;
extern char kernel_stack_bottom;
extern char kernel_stack_top;

/* Function implementations */

void main(uint64_t dtb_addr)
{
    fdt_error_t fdt_error;
    uintptr_t initramfs_begin = CONFIG_INITRAMFS_BASE;
    uintptr_t initramfs_end = CONFIG_INITRAMFS_END;

    /* initialization */
    mini_uart_init();
    init_printf(NULL, printf_putc);
    simple_allocator_init();
    timer_init();

    /*
     * Keep QEMU's default initramfs range as a fallback. The DTB path below
     * replaces that range with /chosen linux,initrd-start/end when the
     * firmware or bootloader provides a valid devicetree address.
     */
    fdt_error = fdt_init(&boot_fdt, (uintptr_t) dtb_addr);
    if (fdt_error == FDT_SUCCESS) {
        initramfs_read_range_from_fdt(&boot_fdt, &initramfs_begin, &initramfs_end);
    }

    initramfs_set_range(initramfs_begin, initramfs_end);

    if (buddy_initialize(fdt_error == FDT_SUCCESS ? &boot_fdt : NULL,
                         initramfs_begin,
                         initramfs_end) != BUDDY_SUCCESS) {
        printf("Buddy system initialization failed.\n");
    } else if (!kernel_allocator_init()) {
        printf("Dynamic allocator initialization failed.\n");
    }

    /* enter simple shell */
    mini_uart_enable_async();
    /*
     * Lab4 prototype currently implements source handling and nesting policy for IRQ.
     * Keep Debug, SError, and FIQ masked until they have complete handlers.
     */
    daif_irq_enable();
    shell_run(fdt_error == FDT_SUCCESS ? &boot_fdt : NULL);
}

static void printf_putc(void *context, char c)
{
    (void) context;
    mini_uart_putc(c);
}

static buddy_error_t buddy_initialize(const fdt_t *fdt,
                                      uintptr_t initramfs_begin,
                                      uintptr_t initramfs_end)
{
    buddy_error_t error;

    error = buddy_init(CONFIG_BUDDY_MEMORY_BASE, CONFIG_BUDDY_MEMORY_END);
    if (error != BUDDY_SUCCESS) {
        printf("buddy_init: %s\n", buddy_error_string(error));
        return error;
    }

    error = buddy_reserve(SPIN_TABLE_BEGIN, SPIN_TABLE_END);
    if (error != BUDDY_SUCCESS) {
        printf("buddy_reserve(spin table): %s\n", buddy_error_string(error));
        return error;
    }

    error = buddy_reserve((uintptr_t) &kernel_stack_bottom, (uintptr_t) &kernel_stack_top);
    if (error != BUDDY_SUCCESS) {
        printf("buddy_reserve(kernel stack): %s\n", buddy_error_string(error));
        return error;
    }

    error = buddy_reserve((uintptr_t) &kernel_begin, (uintptr_t) &kernel_end);
    if (error != BUDDY_SUCCESS) {
        printf("buddy_reserve(kernel image): %s\n", buddy_error_string(error));
        return error;
    }

    /* Reserve the entire startup heap because inherited subsystems still allocate from it. */
    error = buddy_reserve(simple_allocator_begin(), simple_allocator_end());
    if (error != BUDDY_SUCCESS) {
        printf("buddy_reserve(startup heap): %s\n", buddy_error_string(error));
        return error;
    }

    error = buddy_reserve(initramfs_begin, initramfs_end);
    if (error != BUDDY_SUCCESS) {
        printf("buddy_reserve(initramfs): %s\n", buddy_error_string(error));
        return error;
    }

    if (fdt != NULL) {
        error = buddy_reserve((uintptr_t) fdt->base, (uintptr_t) fdt->base + fdt->total_size);
        if (error != BUDDY_SUCCESS) {
            printf("buddy_reserve(fdt): %s\n", buddy_error_string(error));
            return error;
        }
    }

    error = buddy_build();
    if (error != BUDDY_SUCCESS) {
        printf("buddy_build: %s\n", buddy_error_string(error));
    }

    return error;
}
