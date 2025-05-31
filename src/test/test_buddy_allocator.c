#include <test_buddy_allocator.h>
#include <buddy_allocator.h>
#include <stdio.h>
#include <assert.h>

#define TEST_SIZE 1024
#define TEST_NUM_LEVELS 4


int test_buddy_allocator() {
    printf("=== Running Buddy Allocator tests ===\n");

    BuddyAllocator buddy;
    if (BuddyAllocator_create(&buddy, TEST_SIZE, TEST_NUM_LEVELS) == NULL) {
        printf("Failed to create buddy allocator\n");
        return -1;
    }


    // Fill allocated memory with a pattern to verify allocation
    printf("Allocating 100 bytes\n");
    void* ptr1 = BuddyAllocator_alloc(&buddy, 100);
    BuddyAllocator_print_state(&buddy);
    if (ptr1) {
        char* data = (char*)ptr1;
        for (int i = 0; i < 100; i++) {
            data[i] = (char)(i % 256); // Fill with repeating pattern 0-255
        }
        printf("Memory filled with pattern at %p\n", (void*)ptr1);
    }

    printf("Allocating 100 bytes\n");
    void *ptr2 = BuddyAllocator_alloc(&buddy, 100);
    BuddyAllocator_print_state(&buddy);

    printf("Freeing memory at %p\n", (void*)ptr1);
    BuddyAllocator_release(&buddy, ptr1); 
    BuddyAllocator_print_state(&buddy);    

    
    printf("Freeing memory at %p\n", (void*)ptr2);
    BuddyAllocator_release(&buddy, ptr2); 
    BuddyAllocator_print_state(&buddy);    

    if (BuddyAllocator_destroy(&buddy) != 0) {
        printf("Failed to destroy buddy allocator\n");
        return -1;
    }


    printf("All Buddy Allocator tests passed!\n");
    return 0;
}
