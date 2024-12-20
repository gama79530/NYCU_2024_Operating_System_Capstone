#ifndef __LIST_H__
#define __LIST_H__
#include "types.h"

#define LIST_HEAD(name)     list_head_t name = {&(name), &(name)}

// doubly linked circular list
typedef struct list_head{
    struct list_head *prev;
    struct list_head *next;
} list_head_t;

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
    node->prev = node->next = node;
}

static inline bool list_is_head_node(list_head_t *node, list_head_t *head){
    return node == head;
}

static inline void list_append(list_head_t *node, list_head_t *head){
    list_add(node, head->prev, head);
}

#endif