#include "allocator.h"
#include "config.h"
#include "exception.h"
#include "fdt.h"
#include "initramfs.h"
#include "mini_uart.h"
#include "printf.h"
#include "shell.h"
#include "timer.h"
#include "types.h"

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

/* Private data */
static fdt_t boot_fdt;

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

    /* enter simple shell */
    daif_unmask_irq();
    shell_run(fdt_error == FDT_SUCCESS ? &boot_fdt : NULL);
}

static void printf_putc(void *context, char c)
{
    (void) context;
    mini_uart_putc(c);
}
