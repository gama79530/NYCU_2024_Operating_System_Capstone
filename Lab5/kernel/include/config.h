#ifndef __CONFIG_H__
#define __CONFIG_H__

#define VERBOSE                     1

/* for I/O mode */
#define USE_ASYNC_IO                1

/* for shell */
#define SHELL_BUFFER_MAX_SIZE       256
#define SHELL_TOKEN_MAX_NUM         8
#define SHELL_TOKEN_MAX_LEN         32

/* for ramdisk */
#define ramdisk_init                cpio_init

/* for buddy system */
#define SPIN_TABLE_BASE             0x0000
#define SPIN_TABLE_BOUNDARY         0x1000
#define MEMORY_BASE                 0x00000000UL
#define MEMORY_BOUNDARY             0x3C000000UL
#define FRAME_ORDER                 12
#define BUDDY_GROUP_ORDER_LIMIT     16              // exclusive
#define CHUNK_MIN_ORDER             3


/* derivative settings */
/* for buddy system */
#define FRAME_SIZE                  (1L << FRAME_ORDER)
#define FRAME_NUM                   ((MEMORY_BOUNDARY - MEMORY_BASE) >> FRAME_ORDER)
/* for memory system */
#define CHUNK_MIN_SIZE              (1L << CHUNK_MIN_ORDER)
#define POOL_NUM                    (FRAME_ORDER - CHUNK_MIN_ORDER)

#endif