#include "thread.h"

#include "allocator.h"
#include "config.h"
#include "daif.h"
#include "list.h"
#include "printf.h"
#include "util.h"

/* Private types */

#if CONFIG_THREAD_ID_COUNT <= 256
typedef uint8_t thread_id_link_t;
#else
typedef uint16_t thread_id_link_t;
#endif

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

/* Private function declarations */

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
/** Reclaim zombies and yield to runnable threads in the idle loop. */
static void idle_entry(void);
/** Reclaim at most CONFIG_THREAD_REAP_LIMIT zombies, masking IRQ per reclamation. */
static void reap_zombies(void);
/** Initialize the free-ID chain and its head/tail slots with IRQ masked. */
static void thread_ids_init(void);
/** Pop a free ID with IRQ masked; return -1 if the list is empty. */
static int thread_id_allocate(void);
/** Append an allocated ID with IRQ masked; each ID must be released once. */
static void thread_id_release(int id);

/* Private constants */

#define THREAD_ID_HEAD 0
#define THREAD_ID_TAIL 1
#define THREAD_ID_END THREAD_ID_TAIL
#define THREAD_ID_FIRST 2
#define THREAD_STACK_ALIGNMENT 16UL

/* Private data */

static thread_t boot_thread;
static thread_t idle_thread;
/* Runnable queue includes waiting boot/idle threads but excludes current. */
static LIST_HEAD(run_queue);
static LIST_HEAD(zombies);
static size_t allocated_threads;
/* [0] holds head, [1] holds tail; free entries link by index, ending at 1.
 * Empty: head = 1; tail is unspecified and must not be read until non-empty. */
static thread_id_link_t free_id_next[CONFIG_THREAD_ID_COUNT];
static bool initialized;

extern char kernel_stack_bottom;
extern char kernel_stack_top;

/* Function implementations */

static void thread_ids_init(void)
{
    free_id_next[THREAD_ID_HEAD] = THREAD_ID_FIRST;
    free_id_next[THREAD_ID_TAIL] = CONFIG_THREAD_ID_COUNT - 1;
    for (size_t id = THREAD_ID_FIRST; id < CONFIG_THREAD_ID_COUNT - 1; id++) {
        free_id_next[id] = id + 1;
    }
    free_id_next[CONFIG_THREAD_ID_COUNT - 1] = THREAD_ID_END;
}

static int thread_id_allocate(void)
{
    int id = free_id_next[THREAD_ID_HEAD];

    if (id == THREAD_ID_END) {
        return -1;
    }
    free_id_next[THREAD_ID_HEAD] = free_id_next[id];
    return id;
}

static void thread_id_release(int id)
{
    free_id_next[id] = THREAD_ID_END;
    if (free_id_next[THREAD_ID_HEAD] == THREAD_ID_END) {
        free_id_next[THREAD_ID_HEAD] = id;
    } else {
        free_id_next[free_id_next[THREAD_ID_TAIL]] = id;
    }
    free_id_next[THREAD_ID_TAIL] = id;
}

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
    thread->stack_top = end & ~(THREAD_STACK_ALIGNMENT - 1);
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

    thread_ids_init();
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

int thread_create(thread_entry_t entry)
{
    thread_t *thread;
    void *stack;
    int id;
    daif_irq_state_t irq = daif_irq_save();

    if (!initialized || entry == NULL) {
        daif_irq_restore(irq);
        return -1;
    }
    id = thread_id_allocate();
    if (id < 0) {
        daif_irq_restore(irq);
        return -1;
    }

    thread = malloc(sizeof(*thread));
    if (thread == NULL) {
        thread_id_release(id);
        daif_irq_restore(irq);
        return -1;
    }
    stack = malloc(CONFIG_THREAD_STACK_SIZE);
    if (stack == NULL) {
        free(thread);
        thread_id_release(id);
        daif_irq_restore(irq);
        return -1;
    }

    memzero(thread, sizeof(*thread));
    LIST_INIT(&thread->anchor);
    thread->entry = entry;
    thread_set_stack(thread, stack);
    thread->state = THREAD_RUNNABLE;
    thread->id = id;
    thread->context.lr = (uintptr_t) thread_start;
    scheduler_add_thread(thread);
    allocated_threads++;
    daif_irq_restore(irq);
    return id;
}

int thread_current_id(void)
{
    return initialized ? thread_current()->id : -1;
}

size_t thread_count(void)
{
    return allocated_threads;
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
    thread_id_release(thread->id);
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
