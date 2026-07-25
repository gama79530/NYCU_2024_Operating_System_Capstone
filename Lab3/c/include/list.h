#ifndef LAB3_C_LIST_H
#define LAB3_C_LIST_H

#include "types.h"

typedef struct list_head {
    struct list_head *next;
    struct list_head *prev;
} list_head_t;

#define LIST_HEAD(name) list_head_t name = { &(name), &(name) }
#define LIST_INIT(node) ((node)->prev = (node)->next = (node))

#define list_for_each(node, head) \
    for ((node) = (head)->next; (node) != (head); (node) = (node)->next)

#define list_for_each_safe(node, safe, head) \
    for ((node) = (head)->next, (safe) = (node)->next; \
         (node) != (head); \
         (node) = (safe), (safe) = (node)->next)

/* Insert node between prev and next. */
static inline void list_add(list_head_t *node, list_head_t *prev, list_head_t *next)
{
    prev->next = node;
    next->prev = node;
    node->prev = prev;
    node->next = next;
}

/* Insert node as the first element after head. */
static inline void list_add_first(list_head_t *node, list_head_t *head)
{
    list_add(node, head, head->next);
}

/* Insert node as the last element before head. */
static inline void list_add_last(list_head_t *node, list_head_t *head)
{
    list_add(node, head->prev, head);
}

/* Remove node from its current list and reinitialize it as a single-node list. */
static inline void list_remove(list_head_t *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = node;
    node->prev = node;
}

/* Return true when head has no list elements. */
static inline bool list_is_empty(list_head_t *head)
{
    return head->next == head;
}

/* Return true when node is the sentinel head for this list. */
static inline bool list_node_is_head(list_head_t *node, list_head_t *head)
{
    return node == head;
}

#endif
