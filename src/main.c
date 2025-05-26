#include <main.h>

int main() {
    int alloc_tests = test_allocator();
    printf("ALLOCATOR - %d tests completed successfully.\n", alloc_tests);
    
    int list_tests = test_double_linked_list();
    printf("LINKED LIST - %d tests completed successfully.\n", list_tests);
    
    return 0;
}