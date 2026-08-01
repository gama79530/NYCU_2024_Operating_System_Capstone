#ifndef LAB3_C_EXCEPTION_H
#define LAB3_C_EXCEPTION_H

#define EXCEPTION_CLASS_SYNC_VALUE   0
#define EXCEPTION_CLASS_IRQ_VALUE    1
#define EXCEPTION_CLASS_FIQ_VALUE    2
#define EXCEPTION_CLASS_SERROR_VALUE 3

#define EXCEPTION_ORIGIN_CURRENT_SP0_VALUE  0
#define EXCEPTION_ORIGIN_CURRENT_SPX_VALUE  1
#define EXCEPTION_ORIGIN_LOWER_AARCH64_VALUE 2
#define EXCEPTION_ORIGIN_LOWER_AARCH32_VALUE 3

#ifndef __ASSEMBLER__

#include "types.h"

typedef enum {
    EXCEPTION_CLASS_SYNC = EXCEPTION_CLASS_SYNC_VALUE,
    EXCEPTION_CLASS_IRQ = EXCEPTION_CLASS_IRQ_VALUE,
    EXCEPTION_CLASS_FIQ = EXCEPTION_CLASS_FIQ_VALUE,
    EXCEPTION_CLASS_SERROR = EXCEPTION_CLASS_SERROR_VALUE,
} exception_class_t;

typedef enum {
    EXCEPTION_ORIGIN_CURRENT_SP0 = EXCEPTION_ORIGIN_CURRENT_SP0_VALUE,
    EXCEPTION_ORIGIN_CURRENT_SPX = EXCEPTION_ORIGIN_CURRENT_SPX_VALUE,
    EXCEPTION_ORIGIN_LOWER_AARCH64 = EXCEPTION_ORIGIN_LOWER_AARCH64_VALUE,
    EXCEPTION_ORIGIN_LOWER_AARCH32 = EXCEPTION_ORIGIN_LOWER_AARCH32_VALUE,
} exception_origin_t;

typedef struct {
    uint64_t regs[31];
    uint64_t reserved;
    uint64_t spsr_el1;
    uint64_t elr_el1;
} exception_frame_t;

/*
 * Dispatch an exception after the assembly stub has saved the full frame.
 *
 * origin identifies which vector table group was used, and exception_class
 * identifies sync, IRQ, FIQ, or SError.
 */
void exception_dispatch(exception_frame_t *frame,
                        exception_origin_t origin,
                        exception_class_t exception_class);

#endif

#endif
