#include "list.h"

void list_add(list_head_t *node, list_head_t *prev, list_head_t *next){
    prev->next = node;
    node->prev = prev;
    node->next = next;
    next->prev = node;
}

bool list_is_empty(list_head_t *head){
    return head == head->next;
}

void list_remove(list_head_t *node){
    node->next->prev = node->prev;
    node->prev->next = node->next;
}

bool list_is_head_node(list_head_t *node, list_head_t *head){
    return node == head;
}

void list_append(list_head_t *node, list_head_t *head){
    list_add(node, head->prev, head);
}