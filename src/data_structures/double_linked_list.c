#include <data_structures/double_linked_list.h>

DoubleLinkedList* list_create(DoubleLinkedList* list) {
    if (!list) {
        // list = malloc(sizeof(DoubleLinkedList));
        return NULL;
    }
    
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void list_destroy(DoubleLinkedList *list) {
  // Free the list iff dinamically allocated?
    if (!list) return;
    
    Node* current = list->head;
    while (current) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    
    if (list->size == 0 || list->head == NULL) {
        free(list);
    }
}

Node* list_find(DoubleLinkedList* list, Node* item) {
    if (!list || !item) return NULL;
    
    Node* current = list->head;
    while (current) {
        if (current == item) return current;
        current = current->next;
    }
    return NULL;
}

Node* list_insert(DoubleLinkedList* list, Node* previous, Node* item) {
    if (!list || !item) return NULL;
    
    if (!previous) {
        list_push_front(list, item);
        return item;
    }
    
    if (list_find(list, previous) == NULL) return NULL;
    
    item->prev = previous;
    item->next = previous->next;
    
    if (previous->next) {
        previous->next->prev = item;
    } else {
        list->tail = item;
    }
    
    previous->next = item;
    list->size++;
    return item;
}

void list_push_front(DoubleLinkedList *list, Node *item) {
    if (!list || !item) return;
    
    item->prev = NULL;
    item->next = list->head;
    
    if (list->head) {
        list->head->prev = item;
    } else {
        list->tail = item;
    }
    
    list->head = item;
    list->size++;
}

void list_push_back(DoubleLinkedList *list, Node *item) {
    if (!list || !item) return;
    
    item->next = NULL;
    item->prev = list->tail;
    
    if (list->tail) {
        list->tail->next = item;
    } else {
        list->head = item;
    }
    
    list->tail = item;
    list->size++;
}

Node* list_detach(DoubleLinkedList* list, Node* item) {
    if (!list || !item || list->size == 0) return NULL;
    
    if (item->prev) {
        item->prev->next = item->next;
    } else {
        list->head = item->next;
    }
    
    if (item->next) {
        item->next->prev = item->prev;
    } else {
        list->tail = item->prev;
    }
    
    list->size--;
    item->prev = NULL;
    item->next = NULL;
    return item;
}

Node* list_pop_front(DoubleLinkedList *list) {
    if (!list || !list->head) return NULL;
    return list_detach(list, list->head);
}

Node* list_pop_back(DoubleLinkedList *list) {
    if (!list || !list->tail) return NULL;
    return list_detach(list, list->tail);
}