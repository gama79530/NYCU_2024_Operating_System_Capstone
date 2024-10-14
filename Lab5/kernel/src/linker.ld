SECTIONS{
    . = 0x80000;
    kernel_begin = .;

    .text.boot  : { *(.text.boot) }
    .text       : { *(.text) }
    .rodata     : { *(.rodata) }
    .data       : { *(.data) }
    .bss        : { 
        . = ALIGN(0x10);
        bss_begin = .;
        *(.bss*) 
        bss_end = .;
    }

    . = ALIGN(0x10);
    kernel_end = .;
}

bss_size = (bss_end - bss_begin) >> 3;

/* 1 MB startup heap */
startup_heap_base = kernel_end;
startup_heap_boundary = startup_heap_base + 0x100000;
