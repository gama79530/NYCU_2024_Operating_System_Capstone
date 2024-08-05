SECTIONS
{
    . = 0x80000;
    PROVIDE(kernel_begin = .);

    .text.boot  : { *(.text.boot) }
    .text       :  { *(.text) }
    .rodata     : { *(.rodata) }
    .data       : { *(.data) }
    .bss        : { 
        . = ALIGN(0x8);
        bss_begin = .;
        *(.bss*) 
        bss_end = .;
    }

    PROVIDE(kernel_end = .);
}

bss_size = (bss_end - bss_begin) >> 3;