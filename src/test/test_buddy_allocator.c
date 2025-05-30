#include <buddy_allocator.h>
#include <test_buddy_allocator.h>
#ifdef TIME
#include <test_time.h>
#define NUM_RUNS 1
#endif

#define MIN_BLOCK_SIZE (4 * 1024)  // 4KB minimum block size
#define NUM_LEVELS 8               // 8 levels = blocks from 4KB to 512KB
#define MAX_BLOCK_SIZE (MIN_BLOCK_SIZE << (NUM_LEVELS - 1))

int test_buddy_allocator() {
    BuddyAllocator* buddy = BuddyAllocator_create(NULL, NUM_LEVELS, MIN_BLOCK_SIZE);
    if (!buddy) {
        printf("Failed to create buddy allocator\n");
        return -1;
    }
    
    return 0;
}
