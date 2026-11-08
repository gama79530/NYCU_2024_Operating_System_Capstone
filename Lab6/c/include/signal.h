#ifndef LAB6_C_SIGNAL_H
#define LAB6_C_SIGNAL_H

/* Lab6 permits overriding SIGKILL with a user handler, as required by syscall.img. */
#define SIGKILL 9

typedef void (*signal_handler_t)(void);

#endif
