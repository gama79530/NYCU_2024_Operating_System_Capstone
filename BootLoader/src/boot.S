.section ".text.boot"

.global _start
.type _start, %function
_start:
    // Keep the firmware-provided DTB pointer for the loaded kernel.
    mov     x19, x0

    // Only core 0 runs the bootloader. Later labs currently assume single-core execution.
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    cbnz    x0, hang

    // Firmware loads this raw image at 0x80000. The ELF is linked lower, so copy the
    // whole bootloader to its linked address before overwriting 0x80000 with the kernel.
    ldr     x0, =loader_begin
    ldr     x1, =kernel_begin
    ldr     x2, =loader_size
    bl      copy_image
    dsb     sy
    isb

    // Clear relocated BSS.
    ldr     x0, =bss_begin
    ldr     x1, =bss_size
    bl      memzero

    // Put the stack below the relocated bootloader.
    ldr     x0, =loader_begin
    mov     sp, x0

    // Branch to the relocated C entry. adr returns the current physical address, so
    // subtract the relocation offset to reach the copied code.
    mov     x0, x19
    adr     x9, bootloader_main
    ldr     x10, =relocate_offset
    sub     x9, x9, x10
    blr     x9

hang:
    wfe
    b       hang

copy_image:
    cbz     x2, 2f
1:
    ldr     x9, [x1], #8
    str     x9, [x0], #8
    subs    x2, x2, #8
    b.gt    1b
2:
    ret

.size _start, . - _start
