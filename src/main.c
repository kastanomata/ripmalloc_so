#include <stdio.h>
#include "../header/commons.h"
#include "../header/allocator.h"
#include "../header/test/test_allocator.h"

// Test program for allocator functionality
int main() {
    test_allocator();
    printf("All test completed successfully.\n");
    return 0;
}