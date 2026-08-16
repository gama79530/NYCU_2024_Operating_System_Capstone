#ifndef LAB4_C_DAIF_H
#define LAB4_C_DAIF_H

#include "types.h"

typedef uint64_t daif_irq_state_t;

#define DAIF_IRQ_MASK (1 << 7)

/* Allow IRQ exceptions while leaving the other DAIF classes unchanged. */
static inline void daif_irq_enable(void)
{
    asm volatile("msr daifclr, #0x2" ::: "memory");
}

/* Prevent IRQ exceptions while leaving the other DAIF classes unchanged. */
static inline void daif_irq_disable(void)
{
    asm volatile("msr daifset, #0x2" ::: "memory");
}

/* Save DAIF and disable IRQ before entering a critical section. */
static inline daif_irq_state_t daif_irq_save(void)
{
    daif_irq_state_t state;

    asm volatile(
        "mrs %0, daif\n"
        "msr daifset, #0x2"
        : "=r"(state)
        :
        : "memory");
    return state;
}

/* Restore the IRQ mask bit from a state returned by daif_irq_save. */
static inline void daif_irq_restore(daif_irq_state_t state)
{
    if ((state & DAIF_IRQ_MASK) != 0) {
        daif_irq_disable();
    } else {
        daif_irq_enable();
    }
}

#endif
