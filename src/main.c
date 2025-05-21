#include <stdio.h>
#include "../header/commons.h"
#include "../header/allocator.h"
#include "../header/test/test_allocator.h"

// Test program for allocator functionality
int main() {
    int t = test_allocator();
    printf("ALLOCATOR - %i test completed successfully.\n", t);
    return 0;
}