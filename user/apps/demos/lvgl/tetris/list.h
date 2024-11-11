#ifndef __LIST_H__
#define __LIST_H__

#include <stdlib.h>

struct __ListItem;

typedef struct __ListItem {
    struct __ListItem *next;
    int value;
} ListItem;

typedef ListItem* ListHead;

void inline init_list(ListHead *head) {
    *head = NULL;
}

void inline list_insert_into_head(ListHead *head, int value) {
    ListItem *item = malloc(sizeof(*item));
    item->value = value;
    item->next = *head;
    *head = item;
}

void delete_list_item(ListHead *head, int value) {
    ListItem *item = *head;
    if (item == NULL) return;
    if (item->value == value) {
        *head = item->next;
        free(item);
        return;
    }
    while (item->next != NULL) {
        if (item->next->value == value) {
            ListItem *next_next = item->next->next;
            free(item->next);
            item->next = next_next;
            return;
        }
        item = item->next;
    }
}

inline int get_list_head(ListHead *head) {
    if (*head) return (*head)->value;
    return -1;
}

void empty_list(ListHead *head) {
    ListItem *item = *head;
    ListItem *next;
    if (item == NULL) return;
    while (item->next != NULL) {
        next = item->next;
        free(item);
        item = next;
    }
    *head = NULL;
}

inline int list_is_empty(ListHead *head) {
    return *head == NULL;
}

#endif