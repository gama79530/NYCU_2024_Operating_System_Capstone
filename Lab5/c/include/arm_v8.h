#ifndef LAB5_C_ARM_V8_H
#define LAB5_C_ARM_V8_H

/*
 * HCR_EL2, Hypervisor Configuration Register.
 * RW[31] controls the execution state for EL1.
 */
#define HCR_EL2_RW_AARCH64          (1 << 31)
#define HCR_EL2_VALUE               HCR_EL2_RW_AARCH64

/*
 * CNTHCTL_EL2, Counter-timer Hypervisor Control Register.
 * EL1PCTEN and EL1PCEN allow EL1 to access the physical counter/timer.
 */
#define CNTHCTL_EL2_EL1PCTEN        (1 << 0)
#define CNTHCTL_EL2_EL1PCEN         (1 << 1)
#define CNTHCTL_EL2_VALUE           (CNTHCTL_EL2_EL1PCTEN | CNTHCTL_EL2_EL1PCEN)

/*
 * SPSR_EL2, Saved Program Status Register (EL2).
 * M[3:0] = 0101 means returning to EL1h.
 * D/A/I/F mask debug, SError, IRQ and FIQ while entering EL1.
 */
#define SPSR_EL2_MODE_EL1H          (0b0101)
#define SPSR_EL2_MASK_FIQ           (1 << 6)
#define SPSR_EL2_MASK_IRQ           (1 << 7)
#define SPSR_EL2_MASK_SERROR        (1 << 8)
#define SPSR_EL2_MASK_DEBUG         (1 << 9)
#define SPSR_EL2_DAIF_MASKED        (SPSR_EL2_MASK_DEBUG | \
                                     SPSR_EL2_MASK_SERROR | \
                                     SPSR_EL2_MASK_IRQ | \
                                     SPSR_EL2_MASK_FIQ)
#define SPSR_EL2_VALUE              (SPSR_EL2_MODE_EL1H | SPSR_EL2_DAIF_MASKED)

/*
 * SPSR_EL1, Saved Program Status Register (EL1).
 * M[3:0] = 0000 means returning to EL0t.
 */
#define SPSR_EL1_MODE_EL0T          (0b0000)
#define SPSR_EL1_MASK_FIQ           (1 << 6)
#define SPSR_EL1_MASK_IRQ           (1 << 7)
#define SPSR_EL1_MASK_SERROR        (1 << 8)
#define SPSR_EL1_MASK_DEBUG         (1 << 9)
#define SPSR_EL1_DAIF_MASKED        (SPSR_EL1_MASK_DEBUG | \
                                     SPSR_EL1_MASK_SERROR | \
                                     SPSR_EL1_MASK_IRQ | \
                                     SPSR_EL1_MASK_FIQ)
#define SPSR_EL1_EL0T_DAIF_MASKED   (SPSR_EL1_MODE_EL0T | SPSR_EL1_DAIF_MASKED)

#endif
