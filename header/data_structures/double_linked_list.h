#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>


// Generics list 
typedef struct Node {
  struct Node *prev;
  struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    int size;
} DoubleLinkedList;

// initializes list
DoubleLinkedList* list_create(DoubleLinkedList* list);
// destroys list (?) 
// FIXME is this needed?
void list_destroy(DoubleLinkedList *list);

// finds item in list (checks address)
Node* list_find(DoubleLinkedList* list, Node* item);
// inserts item in list (does not initialize item)
Node* list_insert(DoubleLinkedList* head, Node* previous, Node* item);
// puts item at first position
void list_push_front(DoubleLinkedList *list, Node *item);
// puts item at last position
void list_push_back(DoubleLinkedList *list, Node *item);
// removes item from list (checks address)
Node* list_detach(DoubleLinkedList* head, Node* item);
// returns and removes first item from list
Node* list_pop_front(DoubleLinkedList *list);
// returns and removes last item from list
Node* list_pop_back(DoubleLinkedList *list);

