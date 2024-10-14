SECTIONS
{
    . = 0x20000;
    .text       : { *(.text) }
    .rodata     : { *(.rodata) }
    .data       : { *(.data) }
    .bss        : { 
        . = ALIGN(0x8);
        *(.bss*) 
    }
}