#include <test_buddy_allocator.h>
#include <buddy_allocator.h>
#include <stdio.h>
#include <assert.h>

#define TEST_SIZE 1024
#define TEST_NUM_LEVELS 4


int test_buddy_allocator() {
    printf("Running buddy allocator tests...\n");

    BuddyAllocator buddy;
    if (BuddyAllocator_create(&buddy, TEST_SIZE, TEST_NUM_LEVELS) == NULL) {
        printf("Failed to create buddy allocator\n");
        return -1;
    }

    if (BuddyAllocator_destroy(&buddy) != 0) {
        printf("Failed to destroy buddy allocator\n");
        return -1;
    }

    return 0;
}
