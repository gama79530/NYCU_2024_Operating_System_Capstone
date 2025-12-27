/* Program header definitions */
PHDRS {
    text PT_LOAD FLAGS(5);   /* R-X */
    rodata PT_LOAD FLAGS(4); /* R-- */
    data PT_LOAD FLAGS(6);   /* RW- */
}

PROVIDE(kernel_begin = 0x80000);

SECTIONS
{
    /* Kernel load address */
    . = kernel_begin;                            
    .text.boot : {KEEP(*(.text.boot))}      :text   /* Early boot code */
    .text      : ALIGN(0x10) {*(.text)}     :text   /* kernel main code */
    .rodata    : ALIGN(0x10) {*(.rodata)}   :rodata /* Read-only data */
    .data      : ALIGN(0x10) {*(.data)}     :data   /* Initialized data */
    .bss       : ALIGN(0x10) {
        PROVIDE(bss_begin = .);
        *(.bss*)                            
    }                                       :data   /* Zero-initialized data */
    . = ALIGN(0x10);
    PROVIDE(bss_end = .);
    PROVIDE(kernel_end = .);
}

PROVIDE(bss_size = bss_end - bss_begin);
PROVIDE(kernel_size = kernel_end - kernel_begin);
PROVIDE(startup_heap_base = kernel_end);
PROVIDE(startup_heap_boundary = startup_heap_base + 0x100000);  /* 1 MB startup heap */
