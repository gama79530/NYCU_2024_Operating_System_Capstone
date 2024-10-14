#ifndef __LIST_H__
#define __LIST_H__
#include "types.h"

// doubly linked circular list
typedef struct list_head{
    struct list_head *prev;
    struct list_head *next;
} list_head_t;

#define LIST_HEAD(name)     list_head_t name = {&(name), &(name)}

void list_add(list_head_t *node, list_head_t *prev, list_head_t *next);
bool list_is_empty(list_head_t *head);
void list_remove(list_head_t *node);
bool list_is_head_node(list_head_t *node, list_head_t *head);
void list_append(list_head_t *node, list_head_t *head);

#endif