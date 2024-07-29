SECTIONS
{
    . = 0x80000;

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

    . = 0x200000;
    .heap       : {
        . = ALIGN(0x8);
        heap_begin = .;
        heap_end = heap_begin + 0x100000;
    }
}

bss_size = (bss_end - bss_begin) >> 3;