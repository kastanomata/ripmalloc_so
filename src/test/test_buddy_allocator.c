#include <test/test_buddy_allocator.h>

#define PAGE_SIZE sysconf(_SC_PAGESIZE)

void test_buddy_allocator() {
    printf("\n=== Running Buddy Allocator Operations Test ===\n");
    int result = 0;
    BuddyAllocator a;

    if (BuddyAllocator_create(&a, PAGE_SIZE) == NULL) {
        printf("Failed to create allocator\n");
        return;
    }

    void* ptr = BuddyAllocator_alloc(&a, PAGE_SIZE);
    if (!ptr) {
        printf("Failed to allocate memory\n");
        BuddyAllocator_destroy(&a);
        return;
    }

    if (BuddyAllocator_release(&a, ptr, PAGE_SIZE) == -1) {
        printf("Failed to free memory\n");
        result = -1;
    }

    BuddyAllocator_destroy(&a);
    printf(result == 0 ? "All Buddy Allocator tests passed!\n" : 
                        "Some Buddy Allocator tests failed\n");
    return;
}
