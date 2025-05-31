#include <test_buddy_allocator.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_SIZE (1 << 10)  // 1KB
#define NUM_LEVELS 4          // 32KB min block size

// Test initialization with invalid parameters
static int test_invalid_init() {
    BuddyAllocator allocator;
    
    printf("Testing invalid initialization parameters...\n");
    
    // Test with zero total size
    assert(BuddyAllocator_create(&allocator, 0, NUM_LEVELS) == NULL);
    
    // Test with zero levels
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, 0) == NULL);
    
    // Test with NULL allocator
    assert(BuddyAllocator_create(NULL, TOTAL_SIZE, NUM_LEVELS) == NULL);
    
    printf("Invalid initialization test passed\n");
    return 0;
}

// Test basic creation and destruction
static int test_create_destroy() {
    BuddyAllocator allocator;
    
    printf("Testing creation and destruction...\n");
    
    // Test successful creation
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    
    // Verify initial state
    assert(allocator.total_size == TOTAL_SIZE);
    assert(allocator.num_levels == NUM_LEVELS);
    assert(allocator.memory_start != NULL);
    
    // Verify free lists are initialized
    for (int i = 0; i < NUM_LEVELS; i++) {
        assert(allocator.free_lists[i] != NULL);
    }
    
    // Verify first block is in free list
    assert(allocator.free_lists[0]->head != NULL);
    
    // Clean up
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    printf("Creation and destruction test passed\n");
    return 0;
}

// Test allocation and release of single block
static int test_single_allocation() {
    BuddyAllocator allocator;
    void* ptr;
    
    printf("Testing single allocation...\n");
    BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS);
    assert(&allocator != NULL);
    
    // Allocate smallest block size
    size_t alloc_size = allocator.min_block_size;
    printf("alloc_size %ld\n", alloc_size);
    ptr = BuddyAllocator_alloc(&allocator, alloc_size);
    assert(ptr != NULL);
    
    // Verify we can write to the memory
    fill_memory_pattern(ptr, alloc_size, 0xAA);
    assert(verify_memory_pattern(ptr, alloc_size, 0xAA) == 0);
    
    // Release the block
    BuddyAllocator_release(&allocator, ptr);
    
    // Verify allocator state after release
    BuddyAllocator_print_state(&allocator);
    
    // Clean up
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    printf("Single allocation test passed\n");
    return 0;
}

// Test allocation of multiple blocks
static int test_multiple_allocations() {
    BuddyAllocator allocator;
    void* ptrs[16];
    
    printf("Testing multiple allocations...\n");
    
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    size_t alloc_size = allocator.min_block_size;
    alloc_size -= 8;
    printf("alloc_size %ld\n", alloc_size);
    // Allocate all available blocks at smallest size
    int max_blocks = (1 << (NUM_LEVELS-1));  // 2^(levels-1) blocks at min size
    printf("Max blocks: %d\n", max_blocks);
    for (int i = 0; i < max_blocks; i++) {
        ptrs[i] = BuddyAllocator_alloc(&allocator, alloc_size);
        fill_memory_pattern(ptrs[i], alloc_size, i % 256);
        assert(verify_memory_pattern(ptrs[i], alloc_size, i % 256) == 0);
    }
    
    // Try to allocate one more - should fail
    assert(BuddyAllocator_alloc(&allocator, alloc_size) == NULL);
    
    // Release half of the blocks
    for (int i = 0; i < max_blocks/2; i++) {
        BuddyAllocator_release(&allocator, ptrs[i]);
    }
    
    // Allocate again - should succeed
    for (int i = 0; i < max_blocks/2; i++) {
        ptrs[i] = BuddyAllocator_alloc(&allocator, alloc_size);
        fill_memory_pattern(ptrs[i], alloc_size, i % 256);
        assert(verify_memory_pattern(ptrs[i], alloc_size, i % 256) == 0);
    }
    
    // Clean up
    for (int i = 0; i < max_blocks; i++) {
        BuddyAllocator_release(&allocator, ptrs[i]);
    }
    
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    printf("Multiple allocations test passed\n");
    return 0;
}

// Test allocation of different sizes
static int test_varied_sizes() {
    BuddyAllocator allocator;
    
    printf("Testing varied size allocations...\n");
    
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    
    // Calculate sizes for each level
    size_t sizes[NUM_LEVELS];
    for (int i = 0; i < NUM_LEVELS; i++) {
        sizes[i] = TOTAL_SIZE / (1 << i);
    }
    
    // Allocate one block at each level
    void* ptrs[NUM_LEVELS];
    for (int i = 1; i < NUM_LEVELS; i++) {
        printf("Allocating block of size %zu\n", sizes[i] - sizeof(BuddyNode*));
        ptrs[i] = BuddyAllocator_alloc(&allocator, sizes[i] - sizeof(BuddyNode*));
        fill_memory_pattern(ptrs[i], sizes[i] - sizeof(BuddyNode*), 0x55 + i);
        assert(verify_memory_pattern(ptrs[i], sizes[i] - sizeof(BuddyNode*), 0x55 + i) == 0);
    }
    
    // Release blocks
    for (int i = 0; i < NUM_LEVELS; i++) {
        BuddyAllocator_release(&allocator, ptrs[i]);
    }
    
    BuddyAllocator_print_state(&allocator);
    
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    printf("Varied size allocations test passed\n");
    return 0;
}

// Test buddy merging functionality
static int test_buddy_merging() {
    BuddyAllocator allocator;
    
    printf("Testing buddy merging...\n");
    
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    size_t small_size = allocator.min_block_size;
    size_t large_size = small_size * 2;
    
    // Allocate two adjacent small blocks
    void* ptr1 = BuddyAllocator_alloc(&allocator, small_size);
    void* ptr2 = BuddyAllocator_alloc(&allocator, small_size);
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    
    // Release both blocks - they should merge
    BuddyAllocator_release(&allocator, ptr1);
    BuddyAllocator_release(&allocator, ptr2);
    
    // Now we should be able to allocate a large block
    void* large_ptr = BuddyAllocator_alloc(&allocator, large_size);
    assert(large_ptr != NULL);
    
    BuddyAllocator_release(&allocator, large_ptr);
    
    BuddyAllocator_print_state(&allocator);
    
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    printf("Buddy merging test passed\n");
    return 0;
}

// Test invalid releases
static int test_invalid_releases() {
    BuddyAllocator allocator;
    
    printf("Testing invalid releases...\n");
    
    assert(BuddyAllocator_create(&allocator, TOTAL_SIZE, NUM_LEVELS) != NULL);
    
    // Test NULL pointer release
    BuddyAllocator_release(&allocator, NULL);  // Should not crash
    
    // Test double release
    void* ptr = BuddyAllocator_alloc(&allocator, allocator.min_block_size);
    assert(ptr != NULL);
    BuddyAllocator_release(&allocator, ptr);
    BuddyAllocator_release(&allocator, ptr);  // Should not crash
    
    // Test invalid pointer release
    char invalid_ptr[64];
    BuddyAllocator_release(&allocator, invalid_ptr);  // Should not crash
    
    assert(BuddyAllocator_destroy(&allocator) == 0);
    
    printf("Invalid releases test passed\n");
    return 0;
}

// Main test function
int test_buddy_allocator() {
    int result = 0;
    
    printf("=== Starting BuddyAllocator Tests ===\n\n");
    
    result |= test_invalid_init();
    result |= test_create_destroy();
    result |= test_single_allocation();
    result |= test_multiple_allocations();
    result |= test_varied_sizes();
    result |= test_buddy_merging();
    result |= test_invalid_releases();
    
    printf("\n=== BuddyAllocator Tests Complete ===\n");
    
    if (result != 0) {
        printf("\nSome BuddyAllocator tests failed!\n");
    } else {
        printf("\nAll BuddyAllocator tests passed!\n");
    }
    
    return result;
}