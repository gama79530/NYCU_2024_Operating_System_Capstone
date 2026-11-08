#include "mini_uart.h"

#include "config.h"
#include "daif.h"
#include "peripheral.h"
#include "task_queue.h"
#include "types.h"
#include "util.h"

/* Private types */

typedef struct {
    char *data;
    size_t capacity;
    volatile size_t head;
    volatile size_t tail;
} mini_uart_buffer_t;

/* Private function declarations */

/* Return whether buffer contains no bytes. */
static bool mini_uart_buffer_empty(const mini_uart_buffer_t *buffer);

/* Return whether buffer has no room for another byte. */
static bool mini_uart_buffer_full(const mini_uart_buffer_t *buffer);

/* Append one byte to a buffer known to have available space. */
static void mini_uart_buffer_enqueue(mini_uart_buffer_t *buffer, char c);

/* Remove one byte from a buffer known to be non-empty. */
static char mini_uart_buffer_dequeue(mini_uart_buffer_t *buffer);

/* Enable the selected Mini UART interrupt sources. */
static void mini_uart_enable_sources(uint32_t sources);

/* Disable the selected Mini UART interrupt sources. */
static void mini_uart_disable_sources(uint32_t sources);

/* Read one raw byte directly from Mini UART, waiting until data is ready. */
static char mini_uart_getb_polling(void);

/* Write one raw byte directly to Mini UART, waiting until TX is ready. */
static void mini_uart_putb_polling(char c);

/* Read one raw byte from the buffered asynchronous RX path. */
static char mini_uart_getb_async(void);

/* Queue one raw byte on the buffered asynchronous TX path. */
static void mini_uart_putb_async(char c);

/* Move available hardware RX bytes into the software RX buffer. */
static void mini_uart_receive_task(void *data);

/* Move queued software TX bytes into the hardware TX FIFO. */
static void mini_uart_transmit_task(void *data);

/* Private data */

static char rx_storage[CONFIG_MINI_UART_RX_BUFFER_SIZE];
static char tx_storage[CONFIG_MINI_UART_TX_BUFFER_SIZE];

static mini_uart_buffer_t rx_buffer = {
    .data = rx_storage,
    .capacity = CONFIG_MINI_UART_RX_BUFFER_SIZE,
};

static mini_uart_buffer_t tx_buffer = {
    .data = tx_storage,
    .capacity = CONFIG_MINI_UART_TX_BUFFER_SIZE,
};

static bool async_io_enabled;

/* Function implementations */

void mini_uart_init(void)
{
    uint32_t selector;

    /* Map Mini UART to GPIO14 and GPIO15 using alternative function 5. */
    selector = get32(GPFSEL1);
    selector &= ~((7 << 12) | (7 << 15));
    selector |= (2 << 12) | (2 << 15);
    put32(GPFSEL1, selector);

    /* Disable GPIO pull-up/down and latch that setting into GPIO14/15. */
    put32(GPPUD, 0);
    /* The GPIO pull-control signal requires at least 150 setup cycles. */
    wait_cycles(150);
    put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    /* Hold the clock signal long enough for the GPIO pads to sample it. */
    wait_cycles(150);
    /* Remove the clock signal after the pull setting has been applied. */
    put32(GPPUDCLK0, 0);

    put32(AUX_ENABLES, AUX_IRQ_MINI_UART); /* Enable Mini UART register access. */
    put32(AUX_MU_CNTL, 0);                /* Disable RX/TX while configuring. */
    put32(AUX_MU_LCR, 3);                 /* Use 8-bit data mode. */
    put32(AUX_MU_MCR, 0);                 /* Keep the RTS line high. */
    put32(AUX_MU_IER, 0);                 /* Disable RX/TX interrupts initially. */
    put32(AUX_MU_IIR, 0xc6);              /* Enable FIFOs and clear RX/TX data. */
    put32(AUX_MU_BAUD, 270);              /* Set 115200 baud at 250 MHz core clock. */
    put32(AUX_MU_CNTL, 3);                /* Enable the receiver and transmitter. */

    /* Start in polling mode with both software buffers empty. */
    rx_buffer.head = 0;
    rx_buffer.tail = 0;
    tx_buffer.head = 0;
    tx_buffer.tail = 0;
    async_io_enabled = false;
}

void mini_uart_enable_async(void)
{
    daif_irq_state_t state = daif_irq_save();

    rx_buffer.head = 0;
    rx_buffer.tail = 0;
    tx_buffer.head = 0;
    tx_buffer.tail = 0;
    async_io_enabled = true;

    /* Route the AUX peripheral interrupt through the BCM interrupt controller. */
    put32(ENABLE_IRQS_1, IRQ_AUX);
    mini_uart_disable_sources(AUX_MU_IER_TX);
    mini_uart_enable_sources(AUX_MU_IER_RX);

    daif_irq_restore(state);
}

bool mini_uart_irq_pending(void)
{
    return (get32(IRQ_PENDING_1) & IRQ_AUX) != 0 &&
           (get32(AUX_IRQ) & AUX_IRQ_MINI_UART) != 0;
}

void mini_uart_handle_irq(void)
{
    uint32_t interrupt_id = get32(AUX_MU_IIR);

    if ((interrupt_id & AUX_MU_IIR_NO_INTERRUPT) != 0) {
        return;
    }

    switch (interrupt_id & AUX_MU_IIR_ID_MASK) {
    case AUX_MU_IIR_ID_RX:
    case AUX_MU_IIR_ID_RX_TIMEOUT:
        mini_uart_disable_sources(AUX_MU_IER_RX);
        if (!task_queue_push(TASK_PRIORITY_UART_RX, mini_uart_receive_task, NULL)) {
            mini_uart_receive_task(NULL);
        }
        break;
    case AUX_MU_IIR_ID_TX:
        mini_uart_disable_sources(AUX_MU_IER_TX);
        if (!task_queue_push(TASK_PRIORITY_UART_TX, mini_uart_transmit_task, NULL)) {
            mini_uart_transmit_task(NULL);
        }
        break;
    default:
        mini_uart_disable_sources(AUX_MU_IER_RX | AUX_MU_IER_TX);
        break;
    }
}

char mini_uart_getc(void)
{
    char c = async_io_enabled ? mini_uart_getb_async() : mini_uart_getb_polling();

    return c == '\r' ? '\n' : c;
}

void mini_uart_putc(char c)
{
    if (c == '\n') {
        if (async_io_enabled) {
            mini_uart_putb_async('\r');
        } else {
            mini_uart_putb_polling('\r');
        }
    }

    if (async_io_enabled) {
        mini_uart_putb_async(c);
    } else {
        mini_uart_putb_polling(c);
    }
}

void mini_uart_puts(const char *s)
{
    while (*s != '\0') {
        mini_uart_putc(*s++);
    }
}

void mini_uart_putln(const char *s)
{
    mini_uart_puts(s);
    mini_uart_putc('\n');
}

static bool mini_uart_buffer_empty(const mini_uart_buffer_t *buffer)
{
    return buffer->head == buffer->tail;
}

static bool mini_uart_buffer_full(const mini_uart_buffer_t *buffer)
{
    return buffer->head == ((buffer->tail + 1) & (buffer->capacity - 1));
}

static void mini_uart_buffer_enqueue(mini_uart_buffer_t *buffer, char c)
{
    buffer->data[buffer->tail] = c;
    buffer->tail = (buffer->tail + 1) & (buffer->capacity - 1);
}

static char mini_uart_buffer_dequeue(mini_uart_buffer_t *buffer)
{
    char c = buffer->data[buffer->head];

    buffer->head = (buffer->head + 1) & (buffer->capacity - 1);
    return c;
}

static void mini_uart_enable_sources(uint32_t sources)
{
    daif_irq_state_t state = daif_irq_save();

    put32(AUX_MU_IER, get32(AUX_MU_IER) | sources);
    daif_irq_restore(state);
}

static void mini_uart_disable_sources(uint32_t sources)
{
    daif_irq_state_t state = daif_irq_save();

    put32(AUX_MU_IER, get32(AUX_MU_IER) & ~sources);
    daif_irq_restore(state);
}

static char mini_uart_getb_polling(void)
{
    while ((get32(AUX_MU_LSR) & AUX_MU_LSR_DATA_READY) == 0) {
    }

    return (char) (get32(AUX_MU_IO) & 0xff);
}

static void mini_uart_putb_polling(char c)
{
    while ((get32(AUX_MU_LSR) & AUX_MU_LSR_TX_READY) == 0) {
    }

    put32(AUX_MU_IO, (uint8_t) c);
}

static char mini_uart_getb_async(void)
{
    while (true) {
        daif_irq_state_t state = daif_irq_save();

        if (!mini_uart_buffer_empty(&rx_buffer)) {
            char c = mini_uart_buffer_dequeue(&rx_buffer);

            mini_uart_enable_sources(AUX_MU_IER_RX);
            daif_irq_restore(state);
            return c;
        }

        mini_uart_enable_sources(AUX_MU_IER_RX);
        daif_irq_restore(state);
        asm volatile("nop");
    }
}

static void mini_uart_putb_async(char c)
{
    while (true) {
        daif_irq_state_t state = daif_irq_save();

        if (!mini_uart_buffer_full(&tx_buffer)) {
            mini_uart_buffer_enqueue(&tx_buffer, c);
            mini_uart_enable_sources(AUX_MU_IER_TX);
            daif_irq_restore(state);
            return;
        }

        /* Keep masked exception output moving even when no TX IRQ can run. */
        if ((get32(AUX_MU_LSR) & AUX_MU_LSR_TX_READY) != 0) {
            put32(AUX_MU_IO, (uint8_t) mini_uart_buffer_dequeue(&tx_buffer));
        }

        mini_uart_enable_sources(AUX_MU_IER_TX);
        daif_irq_restore(state);
    }
}

static void mini_uart_receive_task(void *data)
{
    (void) data;

    while (!mini_uart_buffer_full(&rx_buffer) &&
           (get32(AUX_MU_LSR) & AUX_MU_LSR_DATA_READY) != 0) {
        mini_uart_buffer_enqueue(&rx_buffer, (char) (get32(AUX_MU_IO) & 0xff));
    }

    if (!mini_uart_buffer_full(&rx_buffer)) {
        mini_uart_enable_sources(AUX_MU_IER_RX);
    }
}

static void mini_uart_transmit_task(void *data)
{
    (void) data;

    while (!mini_uart_buffer_empty(&tx_buffer) &&
           (get32(AUX_MU_LSR) & AUX_MU_LSR_TX_READY) != 0) {
        put32(AUX_MU_IO, (uint8_t) mini_uart_buffer_dequeue(&tx_buffer));
    }

    if (!mini_uart_buffer_empty(&tx_buffer)) {
        mini_uart_enable_sources(AUX_MU_IER_TX);
    }
}
