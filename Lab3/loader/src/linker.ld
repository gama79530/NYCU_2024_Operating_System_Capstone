SECTIONS
{
    _offset = 0x20000;
    . = 0x80000 - _offset;
    PROVIDE(_loader_begin = .);

    .text.boot  : { *(.text.boot) }
    .text       : { *(.text) }
    .rodata     : { *(.rodata) }
    .data       : { *(.data) }
    .bss        : { 
        . = ALIGN(0x8);
        _bss_begin = .;
        *(.bss*) 
        _bss_end = .;
    }

    _loader_end = .;
}

loader_size = (_loader_end - _loader_begin) >> 3;
bss_size = (_bss_end - _bss_begin) >> 3;