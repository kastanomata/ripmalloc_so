#include <test/test_bitmap_buddy_allocator.h>
#include <assert.h>

static int test_invalid_init() {
    BitmapBuddyAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing invalid initialization parameters...\n");
    #endif
    
    // Test with zero total size
    assert(BitmapBuddyAllocator_create(&allocator, 0, 4) == NULL);
    
    // Test with zero levels
    assert(BitmapBuddyAllocator_create(&allocator, 1024, 0) == NULL);
    
    // Test with NULL allocator
    assert(BitmapBuddyAllocator_create(NULL, 1024, 4) == NULL);
    
    #ifdef VERBOSE
    printf("Invalid initialization test passed\n");
    #endif
    return 0;
}

static int test_create_destroy() {
    BitmapBuddyAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing creation and destruction...\n");
    #endif
    
    // Test successful creation
    assert(BitmapBuddyAllocator_create(&allocator, 1024, 4) != NULL);
    
    // Verify initial state
    assert(allocator.memory_size == 1024);
    assert(allocator.num_levels == 4);
    assert(allocator.memory != NULL);
    
    // Verify bitmap is initialized
    assert(allocator.bitmap.num_bits > 0);
    
    // Clean up
    assert(BitmapBuddyAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Create and destroy test passed\n");
    #endif
    return 0;
}

static int test_single_allocation() {
    BitmapBuddyAllocator allocator;
    void* ptr;
    
    #ifdef VERBOSE
    printf("Testing single allocation...\n");
    #endif
    
    assert(BitmapBuddyAllocator_create(&allocator, 1024, 4) != NULL);
    
    // Allocate smallest block size
    size_t alloc_size = allocator.min_bucket_size;
    #ifdef VERBOSE
    printf("Allocating block of size %zu\n", alloc_size);
    #endif
    
    ptr = BitmapBuddyAllocator_malloc(&allocator, alloc_size);
    assert(ptr != NULL);
    
    // Verify we can write to the memory
    fill_memory_pattern(ptr, alloc_size, 0xAA);
    assert(verify_memory_pattern(ptr, alloc_size, 0xAA) == 0);
    
    // Release the block
    BitmapBuddyAllocator_free(&allocator, ptr);
    
    #ifdef VERBOSE
    BitmapBuddyAllocator_print_state(&allocator);
    #endif
    
    // Clean up
    assert(BitmapBuddyAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Single allocation test passed\n");
    #endif
    return 0;
}

static int test_multiple_allocations() {
    BitmapBuddyAllocator allocator;
    void* ptrs[16];
    
    #ifdef VERBOSE
    printf("Testing multiple allocations...\n");
    #endif
    
    assert(BitmapBuddyAllocator_create(&allocator, 1024, 4) != NULL);
    size_t alloc_size = allocator.min_bucket_size; // Account for metadata
    
    #ifdef VERBOSE
    printf("Allocation size: %zu\n", alloc_size);
    printf("Max blocks available: %d\n", (1 << (allocator.num_levels - 1)));
    #endif
    
    // Allocate all available blocks at smallest size
    int max_blocks = (1 << (allocator.num_levels - 1));
    for (int i = 0; i < max_blocks; i++) {
        ptrs[i] = BitmapBuddyAllocator_malloc(&allocator, alloc_size);
        fill_memory_pattern(ptrs[i], alloc_size, i % 256);
        assert(verify_memory_pattern(ptrs[i], alloc_size, i % 256) == 0);
    }
    
    // Try to allocate one more - should fail
    assert(BitmapBuddyAllocator_malloc(&allocator, alloc_size) == NULL);
    
    // Release half of the blocks
    for (int i = 0; i < max_blocks / 2; i++) {
        BitmapBuddyAllocator_free(&allocator, ptrs[i]);
    }
    
    // Allocate again - should succeed
    for (int i = 0; i < max_blocks / 2; i++) {
        ptrs[i] = BitmapBuddyAllocator_malloc(&allocator, alloc_size);
    }
    
    // Clean up
    for (int i = 0; i < max_blocks; i++) {
        if (ptrs[i]) {
            BitmapBuddyAllocator_free(&allocator, ptrs[i]);
        }
    }
    
    #ifdef VERBOSE
    BitmapBuddyAllocator_print_state(&allocator);
    #endif
    
    assert(BitmapBuddyAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Multiple allocations test passed\n");
    #endif
    return 0;
}

static int test_varied_sizes() {
    BitmapBuddyAllocator allocator;
    void* ptrs[16];
    
    #ifdef VERBOSE
    printf("Testing varied size allocations...\n");
    #endif

    assert(BitmapBuddyAllocator_create(&allocator, 1024, 4) != NULL);
    // 1024
    // 512
    // 256
    // 128
    
    // Allocate blocks of different sizes
    size_t sizes[] = {128, 128, 256, 512};
    for (int i = 0; i < 4; i++) {
        size_t request_size = sizes[i] - sizeof(void*); // Account for metadata
        #ifdef VERBOSE
        printf("Allocating block of size %zu (level %d)\n", request_size, i);
        #endif
        
        ptrs[i] = BitmapBuddyAllocator_malloc(&allocator, request_size);
        if (ptrs[i] == NULL) {
            #ifdef DEBUG
            printf(RED "Allocation failed for size %zu\n" RESET, request_size);
            #endif
        } else {
            fill_memory_pattern(ptrs[i], request_size, 0x55 + i);
            assert(verify_memory_pattern(ptrs[i], request_size, 0x55 + i) == 0);
        }
    }

    // Release blocks
    for (int i = 0; i < 4; i++) {
        if (ptrs[i]) {
            BitmapBuddyAllocator_free(&allocator, ptrs[i]);
        }
    }
    
    #ifdef VERBOSE
    BitmapBuddyAllocator_print_state(&allocator);
    #endif
    
    assert(BitmapBuddyAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Varied size allocations test passed\n");
    #endif
    return 0;
}

static int test_buddy_merging() {
    BitmapBuddyAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing buddy merging...\n");
    #endif
    
    assert(BitmapBuddyAllocator_create(&allocator, 1024, 4) != NULL);
    size_t small_size = allocator.min_bucket_size; // Smallest block size
    size_t large_size = small_size * 2; // Size of two small blocks
    
    // Allocate two adjacent small blocks
    void* ptr1 = BitmapBuddyAllocator_malloc(&allocator, small_size);
    void* ptr2 = BitmapBuddyAllocator_malloc(&allocator, small_size);
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    
    // Release both blocks - they should merge
    BitmapBuddyAllocator_free(&allocator, ptr1);
    BitmapBuddyAllocator_free(&allocator, ptr2);
    
    // Now we should be able to allocate a large block
    void* large_ptr = BitmapBuddyAllocator_malloc(&allocator, large_size);
    assert(large_ptr != NULL);
    
    BitmapBuddyAllocator_free(&allocator, large_ptr);
    
    #ifdef VERBOSE
    BitmapBuddyAllocator_print_state(&allocator);
    printf("Buddy merging test passed\n");
    #endif
    
    assert(BitmapBuddyAllocator_destroy(&allocator) == 0);
    return 0;
}

static int test_invalid_releases() {
    BitmapBuddyAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing invalid releases...\n");
    #endif
    
    assert(BitmapBuddyAllocator_create(&allocator, 1024, 4) != NULL);
    
    // Try to release a NULL pointer
    assert(BitmapBuddyAllocator_free(&allocator, NULL) == -1);
    
    // Allocate a block and release it
    void* ptr = BitmapBuddyAllocator_malloc(&allocator, allocator.min_bucket_size);
    assert(ptr != NULL);
    
    // Release the block
    assert(BitmapBuddyAllocator_free(&allocator, ptr) == 0);
    
    // Try to release the same pointer again - should fail
    assert(BitmapBuddyAllocator_free(&allocator, ptr) == -1);
    
    #ifdef VERBOSE
    BitmapBuddyAllocator_print_state(&allocator);
    printf("Invalid releases test passed\n");
    #endif
    
    assert(BitmapBuddyAllocator_destroy(&allocator) == 0);
    
    return 0;
}

int test_bitmap_buddy_allocator() {
    int result = 0;
    
    printf("=== Running BitmapBuddyAllocator Tests ===\n");
    
    result |= test_invalid_init();
    result |= test_create_destroy();
    result |= test_single_allocation();
    result |= test_multiple_allocations();
    result |= test_varied_sizes();
    result |= test_buddy_merging();
    result |= test_invalid_releases();
    
    
    if (result != 0) {
        // Red color for failed tests
        printf("\033[1;31mSome BitmapBuddyAllocator tests failed!\033[0m\n");
    } else {
        // Green color for passed tests
        printf("\033[1;32mAll BitmapBuddyAllocator tests passed!\033[0m\n");
    }
    printf("=== BitmapBuddyAllocator Tests Complete ===\n");
    
    return result;
}