SECTIONS
{
    _offset = 0x20000;
    . = 0x80000 - _offset;
    PROVIDE(loader_begin = .);

    .text.boot  : { *(.text.boot) }
    .text       : { *(.text) }
    .rodata     : { *(.rodata) }
    .data       : { *(.data) }
    .bss        : { 
        . = ALIGN(0x8);
        bss_begin = .;
        *(.bss*) 
        bss_end = .;
    }

    loader_end = .;
}

loader_size = (loader_end - loader_begin) >> 3;
bss_size = (bss_end - bss_begin) >> 3;