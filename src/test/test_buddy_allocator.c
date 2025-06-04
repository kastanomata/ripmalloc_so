#include <test_buddy_allocator.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_SIZE (1 << 10)  // 1KB
#define NUM_LEVELS 4          // Results in 128B min block size (1024/2^4)

// Test initialization with invalid parameters
static int test_invalid_init() {
    BuddyAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing invalid initialization parameters...\n");
    #endif
    
    // Test with zero total size
    assert(BuddyAllocator_create(&allocator, 0, NUM_LEVELS) == NULL);
    
    // Test with zero levels
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, 0) == NULL);
    
    // Test with NULL allocator
    assert(BuddyAllocator_create(NULL, TOTAL_SIZE, NUM_LEVELS) == NULL);
    
    #ifdef VERBOSE
    printf("Invalid initialization test passed\n");
    #endif
    return 0;
}

// Test basic creation and destruction
static int test_create_destroy() {
    BuddyAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing creation and destruction...\n");
    #endif
    
    // Test successful creation
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    
    // Verify initial state
    assert(allocator.total_size == TOTAL_SIZE);
    assert(allocator.num_levels == NUM_LEVELS + 1); // + 1 for the larger block
    assert(allocator.memory_start != NULL);
    
    // Verify free lists are initialized
    for (int i = 0; i < NUM_LEVELS; i++) {
        assert(allocator.free_lists[i] != NULL);
    }
    
    // Verify first block is in free list
    assert(allocator.free_lists[0]->head != NULL);
    
    // Clean up
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Creation and destruction test passed\n");
    #endif
    return 0;
}

// Test allocation and release of single block
static int test_single_allocation() {
    BuddyAllocator allocator;
    void* ptr;
    
    #ifdef VERBOSE
    printf("Testing single allocation...\n");
    #endif
    
    BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS);
    assert(&allocator != NULL);
    
    // Allocate smallest block size
    size_t alloc_size = allocator.min_block_size;
    #ifdef VERBOSE
    printf("Allocating block of size %zu\n", alloc_size);
    #endif
    
    ptr = BuddyAllocator_malloc(&allocator, alloc_size);
    // Verify we can write to the memory
    fill_memory_pattern(ptr, alloc_size, 0xAA);
    assert(verify_memory_pattern(ptr, alloc_size, 0xAA) == 0);
    
    // Release the block
    BuddyAllocator_free(&allocator, ptr);
    
    #ifdef VERBOSE
    BuddyAllocator_print_state(&allocator);
    #endif
    
    // Clean up
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Single allocation test passed\n");
    #endif
    return 0;
}

// Test allocation of multiple blocks
static int test_multiple_allocations() {
    BuddyAllocator allocator;
    int max_blocks = (1 << (NUM_LEVELS));
    void* ptrs[max_blocks];
    
    #ifdef VERBOSE
    printf("Testing multiple allocations...\n");
    #endif

    
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    size_t alloc_size = allocator.min_block_size - sizeof(BuddyNode*); // Account for metadata
    
    #ifdef VERBOSE
    printf("Allocation size: %zu\n", alloc_size);
    printf("Max blocks available: %d\n", (1 << (NUM_LEVELS-1)));
    #endif
    
    // Allocate all available blocks at smallest size
    for (int i = 0; i < max_blocks; i++) {
        ptrs[i] = BuddyAllocator_malloc(&allocator, alloc_size);
        fill_memory_pattern(ptrs[i], alloc_size, i % 256);
        assert(verify_memory_pattern(ptrs[i], alloc_size, i % 256) == 0);
    }
    // Try to allocate one more - should fail
    assert(BuddyAllocator_malloc(&allocator, alloc_size) == NULL);
    
    // Release half of the blocks
    for (int i = 0; i < max_blocks/2; i++) {
        BuddyAllocator_free(&allocator, ptrs[i]);
    }
    
    // Allocate again - should succeed
    for (int i = 0; i < max_blocks/2; i++) {
        ptrs[i] = BuddyAllocator_malloc(&allocator, alloc_size);
    }
    
    // Clean up
    for (int i = 0; i < max_blocks; i++) {
        if (ptrs[i]) {
            BuddyAllocator_free(&allocator, ptrs[i]);
        }
    }
    
    #ifdef VERBOSE
    BuddyAllocator_print_state(&allocator);
    #endif
    
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Multiple allocations test passed\n");
    #endif
    return 0;
}

// Test allocation of different sizes
static int test_varied_sizes() {
    int total_size = PAGESIZE;
    int max_alloc_size = total_size - sizeof(void*);
    int half_alloc_size = total_size / 2 - sizeof(void*);
    int quarter_alloc_size = total_size / 4 - sizeof(void*);
    int eight_alloc_size = total_size / 8 - sizeof(void*);
    void *ptrs[4]; // array of four pointers
    void *ptr;
    
    BuddyAllocator buddy;
    BuddyAllocator_create(&buddy, total_size, DEF_LEVELS_NUMBER);

    #ifdef VERBOSE
    BuddyAllocator_print_state(&buddy);
    #endif

    ptr = BuddyAllocator_malloc(&buddy, max_alloc_size);
    if (ptr == NULL) {
        #ifdef DEBUG
        printf(RED "BuddyAllocator_malloc failed\n" RESET);
        #endif
        return -1;
    }

    fill_memory_pattern(ptr, max_alloc_size, 0xAA);
    #ifdef VERBOSE
    BuddyAllocator_print_state(&buddy);
    printf("Memory filled with pattern 0xAA\n");
    #endif
    assert(verify_memory_pattern(ptr, max_alloc_size, 0xAA) == 0);

    BuddyAllocator_free(&buddy, ptr);
    #ifdef VERBOSE
    BuddyAllocator_print_state(&buddy);
    #endif

    ptrs[0] = BuddyAllocator_malloc(&buddy, half_alloc_size);
    ptrs[1] = BuddyAllocator_malloc(&buddy, quarter_alloc_size);
    ptrs[2] = BuddyAllocator_malloc(&buddy, eight_alloc_size);
    ptrs[3] = BuddyAllocator_malloc(&buddy, eight_alloc_size);

    for(int i = 0; i < 4; i++) {
        if (ptrs[i] == NULL) {
            #ifdef DEBUG
            printf("BuddyAllocator_malloc failed for ptrs[%d]\n", i);
            #endif
            return -1;
        }
        fill_memory_pattern(ptrs[i], (i == 0 ? half_alloc_size : (i == 1 ? quarter_alloc_size : eight_alloc_size)), 0x01 + i);
        assert(verify_memory_pattern(ptrs[i], (i == 0 ? half_alloc_size : (i == 1 ? quarter_alloc_size : eight_alloc_size)), 0x01 + i) == 0);
    }
    #ifdef VERBOSE
    BuddyAllocator_print_state(&buddy);
    printf("Allocated four blocks of memory\n");
    
    print_memory_pattern(buddy.memory_start, total_size);
    printf("Memory pattern printed\n");
    #endif
    
    for(int i = 0; i < 4; i++) {
        BuddyAllocator_free(&buddy, ptrs[i]);
    }

    assert(BuddyAllocator_destroy(&buddy) == 0);
    return 0;
}

// Test buddy merging functionality
static int test_buddy_merging() {
    BuddyAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing buddy merging...\n");
    #endif
    
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    size_t small_size = allocator.min_block_size - sizeof(BuddyNode*);
    size_t large_size = small_size * 2;
    
    // Allocate two adjacent small blocks
    void* ptr1 = BuddyAllocator_malloc(&allocator, small_size);
    void* ptr2 = BuddyAllocator_malloc(&allocator, small_size);
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    
    // Release both blocks - they should merge
    BuddyAllocator_free(&allocator, ptr1);
    BuddyAllocator_free(&allocator, ptr2);
    
    // Now we should be able to allocate a large block
    void* large_ptr = BuddyAllocator_malloc(&allocator, large_size);
    assert(large_ptr != NULL);
    
    BuddyAllocator_free(&allocator, large_ptr);
    
    #ifdef VERBOSE
    BuddyAllocator_print_state(&allocator);
    printf("Buddy merging test passed\n");
    #endif
    
    assert(BuddyAllocator_destroy(&allocator) == 0);
    return 0;
}

// Test invalid releases
static int test_invalid_releases() {
    BuddyAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing invalid releases...\n");
    #endif
    
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    
    // Test NULL pointer release
    BuddyAllocator_free(&allocator, NULL);  // Should not crash
    
    // Test double release
    void* ptr = BuddyAllocator_malloc(&allocator, allocator.min_block_size);
    assert(ptr != NULL);
    BuddyAllocator_free(&allocator, ptr);
    BuddyAllocator_free(&allocator, ptr);  // Should not crash
    
    // Test invalid pointer release
    char invalid_ptr[64];
    BuddyAllocator_free(&allocator, invalid_ptr);  // Should not crash
    
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Invalid releases test passed\n");
    #endif
    return 0;
}

// Main test function
int test_buddy_allocator() {
    int result = 0;
    
    printf("=== Running BuddyAllocator Tests ===\n");
    
    result |= test_invalid_init();
    result |= test_create_destroy();
    result |= test_single_allocation();
    result |= test_multiple_allocations();
    result |= test_varied_sizes();
    result |= test_buddy_merging();
    result |= test_invalid_releases();
    
    
    if (result != 0) {
        // Red color for failed tests
        printf("\033[1;31mSome BuddyAllocator tests failed!\033[0m\n");
    } else {
        // Green color for passed tests
        printf("\033[1;32mAll BuddyAllocator tests passed!\033[0m\n");
    }
    printf("=== BuddyAllocator Tests Complete ===\n");
    
    return result;
}