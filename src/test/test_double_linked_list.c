#include <data_structures/double_linked_list.h>
#include <assert.h>

int test_list_create() {
    DoubleLinkedList list;
    DoubleLinkedList* created = list_create(&list);
    assert(created == &list);
    assert(list.size == 0);
    assert(list.head == NULL);
    assert(list.tail == NULL);
    
    return 0;
}

int test_list_push_pop() {
    DoubleLinkedList list;
    list_create(&list);
    
    Node node1, node2, node3;
    
    list_push_front(&list, &node1);
    assert(list.size == 1);
    assert(list.head == &node1);
    assert(list.tail == &node1);
    
    list_push_back(&list, &node2);
    assert(list.size == 2);
    assert(list.tail == &node2);
    
    list_push_front(&list, &node3);
    assert(list.size == 3);
    assert(list.head == &node3);
    
    Node* popped = list_pop_front(&list);
    assert(popped == &node3);
    assert(list.size == 2);
    
    popped = list_pop_back(&list);
    assert(popped == &node2);
    assert(list.size == 1);
    
    return 0;
}

int test_double_linked_list() {
    printf("Double Linked List Test Program\n");
    printf("Running tests...\n");
    
    int tests_passed = 0;
    int total_tests = 2;
    
    if (test_list_create() == 0) tests_passed++;
    if (test_list_push_pop() == 0) tests_passed++;
    
    printf("%d/%d tests passed\n", tests_passed, total_tests);
    return tests_passed;
}