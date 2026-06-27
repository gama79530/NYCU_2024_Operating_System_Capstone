ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}

PROVIDE(kernel_begin = 0x80000);
PROVIDE(relocate_offset = 0x20000);
PROVIDE(loader_begin = kernel_begin - relocate_offset);

SECTIONS
{
    . = loader_begin;

    .text.boot : ALIGN(0x10) { KEEP(*(.text.boot)) } :text
    .text      : ALIGN(0x10) { *(.text .text.*) }    :text
    .rodata    : ALIGN(0x10) { *(.rodata .rodata.*) }:rodata
    .data      : ALIGN(0x10) { *(.data .data.*) }    :data

    . = ALIGN(0x10);
    PROVIDE(bss_begin = .);
    .bss       : {
        *(.bss .bss.*)
        *(COMMON)
    }                                                :data
    . = ALIGN(0x10);
    PROVIDE(bss_end = .);
    PROVIDE(loader_end = .);

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

PROVIDE(loader_size = loader_end - loader_begin);
PROVIDE(bss_size = bss_end - bss_begin);
