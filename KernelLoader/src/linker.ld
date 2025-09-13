/* Program header definitions */
PHDRS {
    text PT_LOAD FLAGS(5);   /* R-X */
    rodata PT_LOAD FLAGS(4); /* R-- */
    data PT_LOAD FLAGS(6);   /* RW- */
}

PROVIDE(kernel_begin = 0x80000);
PROVIDE(relocate_offset = 0x20000);
PROVIDE(loader_begin = kernel_begin - relocate_offset);

SECTIONS
{
    /* Kernel load address */
    . = loader_begin;
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
    PROVIDE(loader_end = .);
}

PROVIDE(loader_size = loader_end - loader_begin);
PROVIDE(bss_size = bss_end - bss_begin);

