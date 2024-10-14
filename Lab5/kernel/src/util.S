.global get_cpu_id
get_cpu_id:
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    ret

.global get_current_el
get_current_el:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    ret
