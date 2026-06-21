#ifndef LAB1_C_LOG_H
#define LAB1_C_LOG_H

#include "config.h"

#if CONFIG_VERBOSE
#include "printf.h"

#define LOG_VERBOSE(module, ...)      \
    do {                              \
        printf("[%s] ", (module));   \
        printf(__VA_ARGS__);          \
        printf("\n");                \
    } while (0)
#else
#define LOG_VERBOSE(module, ...) ((void) 0)
#endif

#endif
