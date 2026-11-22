#include "exception.h"

/***** Private constants *****/

/***** Private types *****/

/***** Private data *****/

/***** Private function declarations *****/

/*
 * Dispatch an exception after the assembly stub has saved the full frame.
 *
 * origin identifies which vector table group was used, and exception_class
 * identifies sync, IRQ, FIQ, or SError.
 */
void exception_dispatch(exception_frame_t *frame,
                        exception_origin_t origin,
                        exception_class_t exception_class);

/* Dispatch synchronous exceptions. */
static void dispatch_sync_exception(exception_frame_t *frame, exception_origin_t origin);

/* Dispatch IRQ exceptions. */
static void dispatch_irq_exception(exception_frame_t *frame, exception_origin_t origin);

/* Dispatch FIQ exceptions. */
static void dispatch_fiq_exception(exception_frame_t *frame, exception_origin_t origin);

/* Dispatch SError exceptions. */
static void dispatch_serror_exception(exception_frame_t *frame, exception_origin_t origin);

/***** Function implementations *****/

void exception_dispatch(exception_frame_t *frame,
                        exception_origin_t origin,
                        exception_class_t exception_class)
{
    switch (exception_class) {
    case EXCEPTION_CLASS_SYNC:
        dispatch_sync_exception(frame, origin);
        break;
    case EXCEPTION_CLASS_IRQ:
        dispatch_irq_exception(frame, origin);
        break;
    case EXCEPTION_CLASS_FIQ:
        dispatch_fiq_exception(frame, origin);
        break;
    case EXCEPTION_CLASS_SERROR:
        dispatch_serror_exception(frame, origin);
        break;
    default:
        break;
    }
}

static void dispatch_sync_exception(exception_frame_t *frame, exception_origin_t origin) {}

static void dispatch_irq_exception(exception_frame_t *frame, exception_origin_t origin) {}

static void dispatch_fiq_exception(exception_frame_t *frame, exception_origin_t origin) {}

static void dispatch_serror_exception(exception_frame_t *frame, exception_origin_t origin) {}
