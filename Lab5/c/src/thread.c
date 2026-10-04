#include "thread.h"

#include "allocator.h"
#include "arm_v8.h"
#include "config.h"
#include "daif.h"
#include "id_pool.h"
#include "list.h"
#include "printf.h"
#include "timer.h"
#include "util.h"

/* Private types */

typedef struct {
    uint64_t x19_x28[10];
    uint64_t fp; /* x29: frame pointer */
    uint64_t lr; /* x30: link register (return address) */
    uint64_t sp;
} cpu_context_t;

typedef enum {
    THREAD_RUNNABLE,
    THREAD_DEAD,
} thread_state_t;

typedef struct {
    cpu_context_t context;
    list_head_t anchor;
    thread_entry_t entry;
    void *stack_allocation; /* Original malloc address, used by free(). */
    uintptr_t stack_bottom; /* First usable byte, aligned upward. */
    uintptr_t stack_top;    /* One past the usable range, aligned downward. */
    void *user_stack_allocation;
    uintptr_t user_stack_bottom;
    uintptr_t user_stack_top;
    exception_frame_t *user_frame; /* Initial frame at the kernel stack top. */
    int id;
    thread_state_t state;
} thread_t;

_Static_assert(offsetof(thread_t, context) == 0, "context must lead thread metadata");
_Static_assert(offsetof(cpu_context_t, x19_x28[0]) == THREAD_CONTEXT_X19_X20, "x19 offset");
_Static_assert(offsetof(cpu_context_t, x19_x28[2]) == THREAD_CONTEXT_X21_X22, "x21 offset");
_Static_assert(offsetof(cpu_context_t, x19_x28[4]) == THREAD_CONTEXT_X23_X24, "x23 offset");
_Static_assert(offsetof(cpu_context_t, x19_x28[6]) == THREAD_CONTEXT_X25_X26, "x25 offset");
_Static_assert(offsetof(cpu_context_t, x19_x28[8]) == THREAD_CONTEXT_X27_X28, "x27 offset");
_Static_assert(offsetof(cpu_context_t, fp) == THREAD_CONTEXT_FP_LR, "fp offset");
_Static_assert(offsetof(cpu_context_t, lr) == THREAD_CONTEXT_FP_LR + 8, "lr offset");
_Static_assert(offsetof(cpu_context_t, sp) == THREAD_CONTEXT_SP, "sp offset");
_Static_assert(sizeof(cpu_context_t) == THREAD_CONTEXT_SIZE, "context size");

_Static_assert(sizeof(exception_frame_t) == 272, "exception assembly frame size");
_Static_assert(offsetof(exception_frame_t, sp_el0) == 248, "SP_EL0 assembly offset");
_Static_assert(offsetof(exception_frame_t, spsr_el1) == 256, "SPSR assembly offset");

/* Private function declarations */

/** EL0 trampoline that invokes exit when the user entry returns. */
extern void el0_thread_return(void);
/** Allocate metadata, ID and kernel stack with IRQ masked; do not enqueue. */
static thread_t *thread_alloc(void);
/** Release an unpublished thread's allocations and ID with IRQ masked. */
static void thread_discard(thread_t *thread);
/** Allocate an unpublished thread's user stack and prepare its frame/start context.
 * Requires IRQ masked; safe only before the thread has begun execution. */
static bool thread_prepare_user(thread_t *thread);
/** Publish a fully prepared thread to the run queue and count it, with IRQ masked. */
static int thread_publish(thread_t *thread);
/** Enter a prepared user frame without enabling IRQ on a partial context. */
static _Noreturn void thread_user_start(void);
/** Translate a value within the copied stack range to the child's stack. */
static uint64_t relocate_stack_value(uint64_t value, uintptr_t begin,
                                     uintptr_t end, uintptr_t child_begin);

/** Save prev's CPU context and restore next's context with IRQ masked. */
extern void switch_to(thread_t *prev, thread_t *next);

/** Read the current thread pointer from TPIDR_EL1. */
static thread_t *thread_current(void);
/** Record the allocation and its aligned usable bounds, then set the initial SP. */
static void thread_set_stack(thread_t *thread, void *stack);
/**
 * Add one runnable thread (including idle) not already linked in any queue.
 * Requires initialized thread state and IRQ masked. Must not allocate, fail,
 * or switch context. Replace together with scheduler_pick_next() to change policy.
 */
static void scheduler_add_thread(thread_t *thread);
/**
 * Select and unlink the next runnable thread from a non-empty queue.
 * Requires initialized thread state and IRQ masked. Idle is always runnable,
 * ensuring a candidate exists. Must not allocate or switch context.
 */
static thread_t *scheduler_pick_next(void);
/** Enable IRQ, run the thread entry, and exit automatically when it returns. */
static _Noreturn void thread_start(void);
/** Mark a scheduling request; switching waits until outer IRQ exit. */
static void scheduling_tick(void *args);
/** Reclaim zombies and yield to runnable threads in the idle loop. */
static void idle_entry(void);
/** Reclaim at most CONFIG_THREAD_REAP_LIMIT zombies, masking IRQ per reclamation. */
static void reap_zombies(void);

/* Private constants */

#define THREAD_STACK_ALIGNMENT 16UL

/* Private data */

static thread_t boot_thread;
static thread_t idle_thread;
/* Runnable queue includes waiting boot/idle threads but excludes current. */
static LIST_HEAD(run_queue);
static LIST_HEAD(zombies);
static size_t allocated_threads;
static id_pool_t *thread_ids;
static bool initialized;
static bool reschedule_pending;
static bool preemption_started;

extern char kernel_stack_bottom;
extern char kernel_stack_top;

/* Function implementations */

static thread_t *thread_current(void)
{
    thread_t *thread;

    asm volatile("mrs %0, tpidr_el1" : "=r"(thread));
    return thread;
}

static void thread_set_stack(thread_t *thread, void *stack)
{
    uintptr_t begin = (uintptr_t) stack;
    uintptr_t end = begin + CONFIG_THREAD_STACK_SIZE;

    thread->stack_allocation = stack;
    /* Trim both ends within the allocation instead of allocating alignment slack. */
    thread->stack_bottom = align_up(begin, THREAD_STACK_ALIGNMENT);
    thread->stack_top = align_down(end, THREAD_STACK_ALIGNMENT);
    thread->context.sp = thread->stack_top;
}

bool thread_init(void)
{
    void *stack;
    daif_irq_state_t irq = daif_irq_save();

    if (initialized) {
        daif_irq_restore(irq);
        return true;
    }
    if (!kernel_allocator_is_ready()) {
        daif_irq_restore(irq);
        return false;
    }

    /* Use the same allocation size and SP alignment as created threads. */
    stack = malloc(CONFIG_THREAD_STACK_SIZE);
    if (stack == NULL) {
        daif_irq_restore(irq);
        return false;
    }

    /* IDs 0 and 1 belong to boot and idle; pool metadata occupies two slots. */
    thread_ids = id_pool_create(CONFIG_THREAD_ID_COUNT);
    if (thread_ids == NULL) {
        free(stack);
        daif_irq_restore(irq);
        return false;
    }
    LIST_INIT(&boot_thread.anchor);
    boot_thread.id = 0;
    boot_thread.state = THREAD_RUNNABLE;
    boot_thread.stack_bottom = (uintptr_t) &kernel_stack_bottom;
    boot_thread.stack_top = (uintptr_t) &kernel_stack_top;
    LIST_INIT(&idle_thread.anchor);
    idle_thread.id = 1;
    idle_thread.state = THREAD_RUNNABLE;
    idle_thread.entry = idle_entry;
    /* Idle never exits, so its stack remains allocated for the kernel lifetime. */
    thread_set_stack(&idle_thread, stack);
    idle_thread.context.lr = (uintptr_t) thread_start;
    asm volatile("msr tpidr_el1, %0" :: "r"(&boot_thread) : "memory");
    initialized = true;
    /* Boot is current; idle waits alongside all other runnable threads. */
    scheduler_add_thread(&idle_thread);
    daif_irq_restore(irq);
    return true;
}

static void scheduling_tick(void *args)
{
    (void) args;
    reschedule_pending = true;
}

bool thread_start_preemption(void)
{
    daif_irq_state_t irq = daif_irq_save();
    uint64_t interval = timer_counter_frequency() >> 5;

    if (preemption_started) {
        daif_irq_restore(irq);
        return true;
    }
    if (!initialized || interval == 0 ||
        timer_add_timeout(TIMER_UNIT_TICKS, interval, true, scheduling_tick, NULL) < 0) {
        daif_irq_restore(irq);
        return false;
    }
    preemption_started = true;
    daif_irq_restore(irq);
    return true;
}

void thread_preempt(void)
{
    if (reschedule_pending) {
        schedule();
    }
}

static thread_t *thread_alloc(void)
{
    int id = id_pool_alloc(thread_ids);
    thread_t *thread;
    void *stack;

    if (id < 0) {
        return NULL;
    }
    thread = malloc(sizeof(*thread));
    if (thread == NULL) {
        id_pool_release(thread_ids, id);
        return NULL;
    }
    stack = malloc(CONFIG_THREAD_STACK_SIZE);
    if (stack == NULL) {
        free(thread);
        id_pool_release(thread_ids, id);
        return NULL;
    }
    memzero(thread, sizeof(*thread));
    LIST_INIT(&thread->anchor);
    thread_set_stack(thread, stack);
    thread->state = THREAD_RUNNABLE;
    thread->id = id;
    return thread;
}

static void thread_discard(thread_t *thread)
{
    free(thread->user_stack_allocation);
    free(thread->stack_allocation);
    id_pool_release(thread_ids, thread->id);
    free(thread);
}

static int thread_publish(thread_t *thread)
{
    scheduler_add_thread(thread);
    allocated_threads++;
    return thread->id;
}

int thread_create(thread_entry_t entry)
{
    daif_irq_state_t irq = daif_irq_save();
    thread_t *thread;
    int id;

    if (!initialized || entry == NULL || (thread = thread_alloc()) == NULL) {
        daif_irq_restore(irq);
        return -1;
    }
    thread->entry = entry;
    thread->context.lr = (uintptr_t) thread_start;
    id = thread_publish(thread);
    daif_irq_restore(irq);
    return id;
}

static bool thread_prepare_user(thread_t *thread)
{
    uintptr_t begin;

    thread->user_stack_allocation = malloc(CONFIG_USER_STACK_SIZE);
    if (thread->user_stack_allocation == NULL) {
        return false;
    }
    begin = (uintptr_t) thread->user_stack_allocation;
    thread->user_stack_bottom = align_up(begin, THREAD_STACK_ALIGNMENT);
    thread->user_stack_top = align_down(begin + CONFIG_USER_STACK_SIZE, THREAD_STACK_ALIGNMENT);
    thread->user_frame = (exception_frame_t *) (thread->stack_top - sizeof(exception_frame_t));
    memzero(thread->user_frame, sizeof(*thread->user_frame));
    thread->context.sp = (uintptr_t) thread->user_frame;
    thread->context.lr = (uintptr_t) thread_user_start;
    return true;
}

static _Noreturn void thread_user_start(void)
{
    exception_return(thread_current()->user_frame);
}

int thread_create_user(thread_entry_t entry)
{
    daif_irq_state_t irq = daif_irq_save();
    thread_t *thread;
    int id;

    if (!initialized || entry == NULL || (thread = thread_alloc()) == NULL) {
        daif_irq_restore(irq);
        return -1;
    }
    if (!thread_prepare_user(thread)) {
        thread_discard(thread);
        daif_irq_restore(irq);
        return -1;
    }
    thread->entry = entry;
    thread->user_frame->elr_el1 = (uintptr_t) entry;
    thread->user_frame->sp_el0 = thread->user_stack_top;
    thread->user_frame->spsr_el1 = SPSR_EL1_EL0T_DAIF_MASKED & ~SPSR_EL1_MASK_IRQ;
    thread->user_frame->regs[30] = (uintptr_t) el0_thread_return;
    id = thread_publish(thread);
    daif_irq_restore(irq);
    return id;
}

bool thread_is_user(void)
{
    return initialized && thread_current()->user_stack_allocation != NULL;
}

bool thread_user_buffer_valid(uintptr_t address, size_t size)
{
    thread_t *thread = initialized ? thread_current() : NULL;

    return thread != NULL && thread->user_stack_allocation != NULL &&
           address >= thread->user_stack_bottom && address <= thread->user_stack_top &&
           size <= thread->user_stack_top - address;
}

static uint64_t relocate_stack_value(uint64_t value, uintptr_t begin,
                                     uintptr_t end, uintptr_t child_begin)
{
    return value >= begin && value <= end ? child_begin + (value - begin) : value;
}

int thread_fork(exception_frame_t *frame)
{
    daif_irq_state_t irq = daif_irq_save();
    thread_t *parent = initialized ? thread_current() : NULL;
    thread_t *child;
    uintptr_t begin;
    uintptr_t child_begin;
    size_t used;
    int id;

    if (parent == NULL || parent->user_stack_allocation == NULL || frame == NULL ||
        frame->sp_el0 < parent->user_stack_bottom || frame->sp_el0 > parent->user_stack_top ||
        (frame->sp_el0 & (THREAD_STACK_ALIGNMENT - 1)) != 0) {
        daif_irq_restore(irq);
        return -1;
    }
    child = thread_alloc();
    if (child == NULL) {
        daif_irq_restore(irq);
        return -1;
    }
    if (!thread_prepare_user(child)) {
        thread_discard(child);
        daif_irq_restore(irq);
        return -1;
    }
    begin = frame->sp_el0;
    used = parent->user_stack_top - begin;
    if (used > child->user_stack_top - child->user_stack_bottom) {
        thread_discard(child);
        daif_irq_restore(irq);
        return -1;
    }
    child_begin = child->user_stack_top - used;
    memcpy((void *) child_begin, (const void *) begin, used);
    memcpy(child->user_frame, frame, sizeof(*frame));
    /* No MMU: lab stack-only relocation treats stack-range words as pointers.
     * This includes saved FP chains, but cannot distinguish an integer with the
     * same value or find unaligned pointers. It is not a general POSIX fork. */
    for (size_t i = 0; i < used / sizeof(uint64_t); i++) {
        uint64_t *word = &((uint64_t *) child_begin)[i];
        *word = relocate_stack_value(*word, begin, parent->user_stack_top, child_begin);
    }
    for (size_t i = 0; i < 31; i++) {
        child->user_frame->regs[i] = relocate_stack_value(frame->regs[i], begin,
                                                         parent->user_stack_top, child_begin);
    }
    child->user_frame->sp_el0 = child_begin;
    child->user_frame->regs[0] = 0;
    child->entry = parent->entry;
    id = thread_publish(child);
    daif_irq_restore(irq);
    return id;
}

int thread_current_id(void)
{
    return initialized ? thread_current()->id : -1;
}

size_t thread_count(void)
{
    daif_irq_state_t irq = daif_irq_save();
    size_t count = allocated_threads;

    daif_irq_restore(irq);
    return count;
}

static void scheduler_add_thread(thread_t *thread)
{
    /* Round-robin: new and yielding threads join the tail. */
    list_add_last(&thread->anchor, &run_queue);
}

static thread_t *scheduler_pick_next(void)
{
    thread_t *next;

    /* At least one of boot_thread and idle_thread is queued, so the queue is non-empty. */
    next = container_of(run_queue.next, thread_t, anchor);
    list_remove(&next->anchor);
    return next;
}

void schedule(void)
{
    thread_t *prev;
    thread_t *next;
    daif_irq_state_t irq = daif_irq_save();

    if (!initialized) {
        daif_irq_restore(irq);
        return;
    }

    /* A voluntary yield also satisfies any pending scheduling request. */
    reschedule_pending = false;
    prev = thread_current();
    if (prev->state == THREAD_RUNNABLE) {
        scheduler_add_thread(prev);
    }
    next = scheduler_pick_next();

    if (next != prev) {
        switch_to(prev, next);
    }
    /* Each resumed thread restores the IRQ state of its own schedule call. */
    daif_irq_restore(irq);
}

static _Noreturn void thread_start(void)
{
    /* A new thread has no suspended schedule call to restore its IRQ state. */
    daif_irq_enable();
    thread_current()->entry();
    thread_exit();
}

_Noreturn void thread_exit(void)
{
    thread_t *thread;

    daif_irq_disable();
    thread = initialized ? thread_current() : NULL;
    if (thread == NULL || thread == &boot_thread || thread == &idle_thread) {
        printf("thread_exit: boot and idle threads cannot exit.\n");
        while (true) {
            asm volatile("wfe");
        }
    }
    thread->state = THREAD_DEAD;
    /* No wait API retains this ID; idle later frees memory by pointer only. */
    id_pool_release(thread_ids, thread->id);
    thread->id = -1;
    list_add_last(&thread->anchor, &zombies);
    schedule();
    __builtin_unreachable();
}

static void reap_zombies(void)
{
    for (size_t reaped = 0; reaped < CONFIG_THREAD_REAP_LIMIT; reaped++) {
        thread_t *thread;
        daif_irq_state_t irq = daif_irq_save();

        if (list_is_empty(&zombies)) {
            daif_irq_restore(irq);
            break;
        }
        thread = container_of(zombies.next, thread_t, anchor);
        list_remove(&thread->anchor);
        free(thread->user_stack_allocation);
        free(thread->stack_allocation);
        free(thread);
        allocated_threads--;
        daif_irq_restore(irq);
    }
}

static void idle_entry(void)
{
    while (true) {
        reap_zombies();
        schedule();
    }
}
