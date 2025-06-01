#include <test/test_double_linked_list.h>

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

    printf("=== Running Double Linked List Tests ===\n");

    int tests_passed = 0;
    int total_tests = 2;
    
    if (test_list_create() == 0) tests_passed++;
    if (test_list_push_pop() == 0) tests_passed++;
    
    if (tests_passed != total_tests) {
        printf("\033[1;31mSome Double Linked List tests failed!\033[0m\n");
    } else {
        printf("\033[1;32mAll Double Linked List tests passed!\033[0m\n");
    }
    printf("=== Double Linked List Tests Complete ===\n");
    return tests_passed;
}