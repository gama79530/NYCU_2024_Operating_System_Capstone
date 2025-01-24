#ifndef __LIST_H__
#define __LIST_H__
#include "types.h"

// doubly linked circular list
typedef struct list_head{
    struct list_head *prev;
    struct list_head *next;
} list_head_t;

#define LIST_HEAD(name)     list_head_t name = {&(name), &(name)}
#define LIST_INIT(node)     ((node)->prev = (node)->next = (node))

#define list_for_each(node, head) \
    for(node = (head)->next; node != (head); node = node->next)

#define list_reverse_for_each(node, head) \
    for(node = (head)->prev; node != (head); node = node->prev)

#define list_for_each_safe(node, safe, head) \
    for(node = (head)->next, safe = node->next; node != (head); \
         node = safe, safe = node->next)

#define list_reverse_for_each_safe(node, safe, head) \
    for(node = (head)->prev, safe = node->prev; node != (head); \
         node = safe, safe = node->prev)

static inline void list_add(list_head_t *node, list_head_t *prev, list_head_t *next){
    prev->next = node;
    next->prev = node;
    node->prev = prev;
    node->next = next;
}

static inline bool list_is_empty(list_head_t *head){
    return head == head->next;
}

static inline void list_remove(list_head_t *node){
    node->next->prev = node->prev;
    node->prev->next = node->next;
    LIST_INIT(node);
}

static inline bool list_is_head_node(list_head_t *node, list_head_t *head){
    return node == head;
}

static inline void list_add_last(list_head_t *node, list_head_t *head){
    list_add(node, head->prev, head);
}

static inline void list_add_first(list_head_t *node, list_head_t *head){
    list_add(node, head, head->next);
}

#endif