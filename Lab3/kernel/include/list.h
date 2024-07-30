#ifndef LIST_H
#define LIST_H

// doubly linked circular list
typedef struct list_anchor{
    list_anchor_t *next;
    list_anchor_t *prev;
} list_anchor_t;

#define LIST_HEAD(name)     list_anchor_t name = {&(name), &(name)};

static inline void list_add(list_anchor_t *node, list_anchor_t *prev, list_anchor_t *next){
    prev->next = node;
    next->prev = node;
    node->prev = prev;
    node->next = next;
}

static inline int list_is_empty(list_anchor_t *list){
    return list == list->next;
}

static inline void list_remove(list_anchor_t *node){
    node->next->prev = node->prev;
    node->prev->next = node->next;
}

#endif