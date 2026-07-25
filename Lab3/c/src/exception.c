#include "exception.h"

#include "printf.h"
#include "task_queue.h"

#define ESR_EL1_EC_SHIFT 26
#define ESR_EL1_EC_MASK  0x3f
#define ESR_EL1_EC_SVC64 0x15
#define ESR_EL1_ISS_MASK 0x01ffffff

static const char *exception_origin_name(exception_origin_t origin);

static uint64_t read_esr_el1(void)
{
    uint64_t value;

    asm volatile("mrs %0, esr_el1" : "=r"(value));
    return value;
}

static void print_u64_hex(uint64_t value)
{
    uint32_t high = (uint32_t) (value >> 32);
    uint32_t low = (uint32_t) value;

    if (high == 0) {
        printf("0x%x", low);
        return;
    }

    printf("0x%x%08x", high, low);
}

static void handle_svc_demo(exception_frame_t *frame, exception_origin_t origin, uint64_t esr)
{
    printf("SVC #%u from %s: x0 = %u\n",
           (uint32_t) (esr & ESR_EL1_ISS_MASK),
           exception_origin_name(origin),
           (uint32_t) frame->regs[0]);
    printf("SPSR_EL1 = ");
    print_u64_hex(frame->spsr_el1);
    printf("\n");
    printf("ELR_EL1  = ");
    print_u64_hex(frame->elr_el1);
    printf("\n");
    printf("ESR_EL1  = ");
    print_u64_hex(esr);
    printf("\n");
}

static const char *exception_origin_name(exception_origin_t origin)
{
    switch (origin) {
    case EXCEPTION_ORIGIN_CURRENT_SP0:
        return "current_sp0";
    case EXCEPTION_ORIGIN_CURRENT_SPX:
        return "current_spx";
    case EXCEPTION_ORIGIN_LOWER_AARCH64:
        return "lower_aarch64";
    case EXCEPTION_ORIGIN_LOWER_AARCH32:
        return "lower_aarch32";
    default:
        return "unknown";
    }
}

static void handle_default_sync_exception(exception_frame_t *frame,
                                          exception_origin_t origin,
                                          uint64_t esr)
{
    printf("Unhandled sync exception from %s: esr=", exception_origin_name(origin));
    print_u64_hex(esr);
    printf(" elr=");
    print_u64_hex(frame->elr_el1);
    printf("\n");
}

static void dispatch_sync_exception(exception_frame_t *frame, exception_origin_t origin)
{
    uint64_t esr = read_esr_el1();
    uint64_t ec = (esr >> ESR_EL1_EC_SHIFT) & ESR_EL1_EC_MASK;

    if (ec == ESR_EL1_EC_SVC64) {
        handle_svc_demo(frame, origin, esr);
        return;
    }

    handle_default_sync_exception(frame, origin, esr);
}

static void dispatch_irq_exception(exception_origin_t origin)
{
    printf("Unhandled IRQ from %s\n", exception_origin_name(origin));
    task_queue_run();
}

static void handle_fiq_exception(exception_origin_t origin)
{
    printf("Unhandled FIQ from %s\n", exception_origin_name(origin));
}

static void handle_serror_exception(exception_origin_t origin)
{
    printf("Unhandled SError from %s\n", exception_origin_name(origin));
}

void exception_dispatch(exception_frame_t *frame,
                        exception_origin_t origin,
                        exception_class_t exception_class)
{
    switch (exception_class) {
    case EXCEPTION_CLASS_SYNC:
        dispatch_sync_exception(frame, origin);
        break;
    case EXCEPTION_CLASS_IRQ:
        dispatch_irq_exception(origin);
        break;
    case EXCEPTION_CLASS_FIQ:
        handle_fiq_exception(origin);
        break;
    case EXCEPTION_CLASS_SERROR:
        handle_serror_exception(origin);
        break;
    default:
        printf("Unhandled exception class: %u\n", (uint32_t) exception_class);
        break;
    }
}
