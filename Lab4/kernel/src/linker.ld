SECTIONS
{
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
heap_base = kernel_end;
heap_boundary = kernel_end + 0x100000;
