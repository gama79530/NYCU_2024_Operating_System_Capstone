#include "timer.h"

#include "allocator.h"
#include "config.h"
#include "daif.h"
#include "id_pool.h"
#include "list.h"
#include "peripheral.h"
#include "util.h"

/* Private types */

typedef struct timer_event {
    list_head_t anchor;
    uint64_t expires_at;
    uint64_t interval_ticks; /* 0: one-shot; otherwise delay after callback returns. */
    int id;
    timer_callback_t callback;
    void *args;
} timer_event_t;

/* Private function declarations */

/** Read the physical counter frequency in ticks per second. */
static uint64_t read_cntfrq_el0(void);

/** Read the current physical counter value. */
static uint64_t read_cntpct_el0(void);

/** Enable or disable the physical timer. */
static void write_cntp_ctl_el0(uint64_t value);

/** Program the physical timer to fire after delta ticks. */
static void write_cntp_tval_el0(uint64_t delta);

/** Allocate a timer event from the local cache or kernel allocator. */
static timer_event_t *timer_alloc_event(void);

/** Clean up an unlinked event with IRQ masked, then release its ID and storage. */
static void timer_free_event(timer_event_t *event);

/** Insert event by expiration time while preserving FIFO order for ties. */
static void timer_insert_event(timer_event_t *event);

/** Run expired callbacks, then requeue periodic events or recycle one-shots. */
static void timer_run_expired_events(void);

/** Return the next head delta, or one tick if it is already due. */
static bool timer_get_next_delta(uint64_t *delta);

/** Program the hardware timer for the earliest waiting event. */
static void timer_program_next_event(void);

/** Convert a positive duration to ticks, rounding milliseconds upward. */
static bool timer_duration_to_ticks(timer_time_unit_t unit, uint64_t duration, uint64_t *ticks);

/* Private data */

static LIST_HEAD(waiting_events);
static LIST_HEAD(running_events);
static LIST_HEAD(free_events);
static id_pool_t *event_ids;
static size_t cached_count;
static uint64_t counter_frequency;

/* Function implementations */

bool timer_init(void)
{
    uint64_t frequency = read_cntfrq_el0();

    write_cntp_ctl_el0(0);
    if (frequency == 0) {
        return false;
    }
    event_ids = id_pool_create(CONFIG_TIMER_MAX_EVENTS + 2);
    if (event_ids == NULL) {
        return false;
    }
    /* A nonzero frequency also indicates that event ID allocation is ready. */
    counter_frequency = frequency;

    /* Route the EL1 physical non-secure timer IRQ to core 0. */
    put32(CORE0_TIMER_IRQ_CTRL, CORE_TIMER_IRQ_CTRL_CNTPNSIRQ);
    return true;
}

bool timer_irq_pending(void)
{
    return (get32(CORE0_IRQ_SOURCE) & CORE_TIMER_IRQ_CTRL_CNTPNSIRQ) != 0;
}

void timer_handle_irq(void)
{
    write_cntp_ctl_el0(0);
    timer_run_expired_events();
    timer_program_next_event();
}

int timer_add_timeout(timer_time_unit_t unit, uint64_t duration, bool periodic,
                      timer_callback_t callback, void *args)
{
    timer_event_t *event;
    daif_irq_state_t irq;
    uint64_t now;
    uint64_t ticks;
    int id;

    if (duration == 0 || callback == NULL ||
        !timer_duration_to_ticks(unit, duration, &ticks)) {
        return -1;
    }

    irq = daif_irq_save();
    now = read_cntpct_el0();
    if (ticks > UINT64_MAX - now) {
        daif_irq_restore(irq);
        return -1;
    }
    event = timer_alloc_event();
    if (event == NULL) {
        daif_irq_restore(irq);
        return -1;
    }

    event->expires_at = now + ticks;
    event->interval_ticks = periodic ? ticks : 0;
    event->callback = callback;
    event->args = args;
    id = event->id;
    timer_insert_event(event);

    if (waiting_events.next == &event->anchor) {
        timer_program_next_event();
    }
    daif_irq_restore(irq);
    return id;
}

bool timer_stop_periodic(int id)
{
    list_head_t *queues[] = { &waiting_events, &running_events };
    daif_irq_state_t irq = daif_irq_save();

    for (size_t i = 0; i < sizeof(queues) / sizeof(queues[0]); i++) {
        list_head_t *cursor;

        list_for_each(cursor, queues[i]) {
            timer_event_t *event = container_of(cursor, timer_event_t, anchor);
            if (event->id == id) {
                /* Finish the pending/current invocation as a one-shot event. */
                event->interval_ticks = 0;
                daif_irq_restore(irq);
                return true;
            }
        }
    }
    daif_irq_restore(irq);
    return false;
}

uint64_t timer_current_seconds(void)
{
    if (counter_frequency == 0) {
        return 0;
    }

    return read_cntpct_el0() / counter_frequency;
}

static uint64_t read_cntfrq_el0(void)
{
    uint64_t value;

    asm volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

static uint64_t read_cntpct_el0(void)
{
    uint64_t value;

    asm volatile("mrs %0, cntpct_el0" : "=r"(value));
    return value;
}

static void write_cntp_ctl_el0(uint64_t value)
{
    asm volatile("msr cntp_ctl_el0, %0" ::"r"(value) : "memory");
}

static void write_cntp_tval_el0(uint64_t delta)
{
    asm volatile("msr cntp_tval_el0, %0" ::"r"(delta) : "memory");
}

static timer_event_t *timer_alloc_event(void)
{
    timer_event_t *event;
    int id;

    id = id_pool_alloc(event_ids);
    if (id < 0) {
        return NULL;
    }

    if (!list_is_empty(&free_events)) {
        event = container_of(free_events.next, timer_event_t, anchor);
        list_remove(&event->anchor);
        cached_count--;
    } else {
        event = malloc(sizeof(*event));
        if (event == NULL) {
            id_pool_release(event_ids, id);
            return NULL;
        }
        LIST_INIT(&event->anchor);
    }

    event->id = id;
    return event;
}

static void timer_free_event(timer_event_t *event)
{
    if (event == NULL) {
        return;
    }

    /* Argument data belongs to one allocation; any running callback has returned. */
    free(event->args);
    id_pool_release(event_ids, event->id);
    event->id = -1;
    event->interval_ticks = 0;
    event->expires_at = 0;
    event->callback = NULL;
    event->args = NULL;

    if (!kernel_allocator_is_ready() || cached_count < CONFIG_TIMER_EVENT_CACHE_SIZE) {
        list_add_last(&event->anchor, &free_events);
        cached_count++;
    } else {
        free(event);
    }
}

static void timer_insert_event(timer_event_t *event)
{
    list_head_t *cursor;

    list_for_each(cursor, &waiting_events) {
        timer_event_t *queued_event = container_of(cursor, timer_event_t, anchor);
        if (queued_event->expires_at > event->expires_at) {
            list_add(&event->anchor, cursor->prev, cursor);
            return;
        }
    }

    list_add_last(&event->anchor, &waiting_events);
}

static void timer_run_expired_events(void)
{
    while (!list_is_empty(&waiting_events)) {
        timer_event_t *event = container_of(waiting_events.next, timer_event_t, anchor);
        uint64_t now = read_cntpct_el0();

        if (event->expires_at > now) {
            return;
        }

        list_remove(&event->anchor);
        list_add_last(&event->anchor, &running_events);

        daif_irq_enable();
        event->callback(event->args);
        daif_irq_disable();

        list_remove(&event->anchor);
        if (event->interval_ticks != 0) {
            now = read_cntpct_el0();
            /* Saturate an unrepresentable deadline; retain the event until its final invocation. */
            event->expires_at = event->interval_ticks > UINT64_MAX - now
                                ? UINT64_MAX : now + event->interval_ticks;
            timer_insert_event(event);
        } else {
            timer_free_event(event);
        }
    }
}

static void timer_program_next_event(void)
{
    uint64_t delta;

    if (!timer_get_next_delta(&delta)) {
        write_cntp_ctl_el0(0);
        return;
    }

    /* TVAL is a signed 32-bit countdown; revisit distant deadlines in chunks. */
    write_cntp_tval_el0(delta > INT32_MAX ? INT32_MAX : delta);
    write_cntp_ctl_el0(1);
}

static bool timer_get_next_delta(uint64_t *delta)
{
    timer_event_t *event;
    uint64_t now;

    if (list_is_empty(&waiting_events)) {
        return false;
    }
    event = container_of(waiting_events.next, timer_event_t, anchor);
    now = read_cntpct_el0();
    *delta = event->expires_at > now ? event->expires_at - now : 1;
    return true;
}

static bool timer_duration_to_ticks(timer_time_unit_t unit, uint64_t duration, uint64_t *ticks)
{
    uint64_t seconds;
    uint64_t fraction = 0;

    if (counter_frequency == 0) {
        return false;
    }
    switch (unit) {
    case TIMER_UNIT_TICKS:
        *ticks = duration;
        return true;
    case TIMER_UNIT_SECONDS:
        seconds = duration;
        break;
    case TIMER_UNIT_MILLISECONDS: {
        uint64_t remainder = duration % 1000;
        seconds = duration / 1000;
        /* Split before multiplying to avoid overflow; round fractional ticks up. */
        fraction = remainder * (counter_frequency / 1000)
                   + (remainder * (counter_frequency % 1000) + 999) / 1000;
        break;
    }
    default:
        return false;
    }
    if (seconds > (UINT64_MAX - fraction) / counter_frequency) {
        return false;
    }
    *ticks = seconds * counter_frequency + fraction;
    return true;
}
