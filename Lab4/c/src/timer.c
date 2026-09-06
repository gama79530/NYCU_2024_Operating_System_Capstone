#include "timer.h"

#include "allocator.h"
#include "config.h"
#include "daif.h"
#include "list.h"
#include "peripheral.h"
#include "string.h"
#include "util.h"

/* Private types */

typedef struct timer_event {
    list_head_t anchor;
    uint64_t expires_at;
    timer_callback_t callback;
    char message[CONFIG_TIMER_MESSAGE_SIZE];
} timer_event_t;

/* Private function declarations */

/* Read the physical counter frequency in ticks per second. */
static uint64_t read_cntfrq_el0(void);

/* Read the current physical counter value. */
static uint64_t read_cntpct_el0(void);

/* Enable or disable the physical timer. */
static void write_cntp_ctl_el0(uint64_t value);

/* Program the physical timer to fire after delta ticks. */
static void write_cntp_tval_el0(uint64_t delta);

/* Allocate a timer event from the local cache or kernel allocator. */
static timer_event_t *timer_alloc_event(void);

/* Reset a timer event and cache or release it. */
static void timer_free_event(timer_event_t *event);

/* Copy message into the fixed-size event buffer. */
static void timer_copy_message(timer_event_t *event, const char *message);

/* Insert event by expiration time while preserving FIFO order for ties. */
static bool timer_insert_event(timer_event_t *event);

/* Run and recycle all currently expired timer events. */
static void timer_run_expired_events(void);

/* Drain expired events and return the next unexpired head delta. */
static bool timer_get_next_delta(uint64_t *delta);

/* Program the hardware timer for the earliest waiting event. */
static void timer_program_next_event(void);

/* Convert a seconds duration to physical counter ticks. */
static bool timer_seconds_to_ticks(uint64_t seconds, uint64_t *ticks);

/* Private data */

static LIST_HEAD(waiting_events);
static LIST_HEAD(free_events);
static size_t active_count;
static size_t cached_count;
static uint64_t counter_frequency;

/* Function implementations */

void timer_init(void)
{
    counter_frequency = read_cntfrq_el0();

    /* Route the EL1 physical non-secure timer IRQ to core 0. */
    put32(CORE0_TIMER_IRQ_CTRL, CORE_TIMER_IRQ_CTRL_CNTPNSIRQ);
    write_cntp_ctl_el0(0);
}

bool timer_irq_pending(void)
{
    return (get32(CORE0_IRQ_SOURCE) & CORE_TIMER_IRQ_CTRL_CNTPNSIRQ) != 0;
}

void timer_handle_irq(void)
{
    write_cntp_ctl_el0(0);
    timer_program_next_event();
}

bool timer_add_timeout(uint64_t seconds, const char *message, timer_callback_t callback)
{
    timer_event_t *event;
    daif_irq_state_t daif_state;
    uint64_t now;
    uint64_t duration;

    if (seconds == 0 || message == NULL || callback == NULL || counter_frequency == 0) {
        return false;
    }

    if (!timer_seconds_to_ticks(seconds, &duration)) {
        return false;
    }

    now = read_cntpct_el0();
    if (UINT64_MAX - now < duration) {
        return false;
    }

    daif_state = daif_irq_save();

    event = timer_alloc_event();
    if (event == NULL) {
        daif_irq_restore(daif_state);
        return false;
    }

    event->expires_at = now + duration;
    event->callback = callback;
    timer_copy_message(event, message);

    if (!timer_insert_event(event)) {
        timer_free_event(event);
        daif_irq_restore(daif_state);
        return false;
    }

    if (waiting_events.next == &event->anchor) {
        timer_program_next_event();
    }

    daif_irq_restore(daif_state);
    return true;
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

    if (active_count >= CONFIG_TIMER_MAX_EVENTS) {
        return NULL;
    }

    if (!list_is_empty(&free_events)) {
        event = container_of(free_events.next, timer_event_t, anchor);
        list_remove(&event->anchor);
        cached_count--;
    } else {
        event = malloc(sizeof(*event));
        if (event == NULL) {
            return NULL;
        }
        LIST_INIT(&event->anchor);
    }

    active_count++;
    return event;
}

static void timer_free_event(timer_event_t *event)
{
    if (event == NULL) {
        return;
    }

    event->expires_at = 0;
    event->callback = NULL;
    event->message[0] = '\0';
    active_count--;

    if (!kernel_allocator_is_ready() || cached_count < CONFIG_TIMER_EVENT_CACHE_SIZE) {
        list_add_last(&event->anchor, &free_events);
        cached_count++;
    } else {
        free(event);
    }
}

static void timer_copy_message(timer_event_t *event, const char *message)
{
    size_t length = strlen(message);

    if (length >= CONFIG_TIMER_MESSAGE_SIZE) {
        length = CONFIG_TIMER_MESSAGE_SIZE - 1;
    }

    for (size_t i = 0; i < length; i++) {
        event->message[i] = message[i];
    }
    event->message[length] = '\0';
}

static bool timer_insert_event(timer_event_t *event)
{
    list_head_t *cursor;

    if (event == NULL || event->callback == NULL) {
        return false;
    }

    list_for_each(cursor, &waiting_events) {
        timer_event_t *queued_event = container_of(cursor, timer_event_t, anchor);
        if (queued_event->expires_at > event->expires_at) {
            list_add(&event->anchor, cursor->prev, cursor);
            return true;
        }
    }

    list_add_last(&event->anchor, &waiting_events);
    return true;
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

        daif_irq_enable();
        event->callback(event->message, timer_current_seconds());
        daif_irq_disable();

        timer_free_event(event);
    }
}

static void timer_program_next_event(void)
{
    uint64_t delta;

    if (!timer_get_next_delta(&delta)) {
        write_cntp_ctl_el0(0);
        return;
    }

    write_cntp_tval_el0(delta);
    write_cntp_ctl_el0(1);
}

static bool timer_get_next_delta(uint64_t *delta)
{
    while (true) {
        timer_event_t *event;
        uint64_t now;

        timer_run_expired_events();

        if (list_is_empty(&waiting_events)) {
            return false;
        }

        event = container_of(waiting_events.next, timer_event_t, anchor);
        now = read_cntpct_el0();
        if (event->expires_at > now) {
            *delta = event->expires_at - now;
            return true;
        }
    }
}

static bool timer_seconds_to_ticks(uint64_t seconds, uint64_t *ticks)
{
    if (ticks == NULL || counter_frequency == 0 || seconds > UINT64_MAX / counter_frequency) {
        return false;
    }

    *ticks = seconds * counter_frequency;
    return true;
}
