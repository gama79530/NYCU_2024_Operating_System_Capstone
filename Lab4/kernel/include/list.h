#ifndef LIST_H
#define LIST_H

// doubly linked circular list
typedef struct list_head{
    struct list_head *prev;
    struct list_head *next;
} list_head_t;

#define LIST_HEAD(name)         list_head_t name = {&(name), &(name)}

static inline void list_add(list_head_t *node, list_head_t *prev, list_head_t *next){
    prev->next = node;
    node->prev = prev;
    node->next = next;
    next->prev = node;
}

static inline int list_is_empty(list_head_t *head){
    return head == head->next;
}

static inline void list_remove(list_head_t *node){
    node->next->prev = node->prev;
    node->prev->next = node->next;
}

static inline int list_node_is_head(list_head_t *node, list_head_t *head){
    return node == head;
}

#endif