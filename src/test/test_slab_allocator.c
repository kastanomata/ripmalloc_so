#include <test_slab_allocator.h>
#define SLAB_SIZE 1024
#define NUM_SLABS 10
#define NUM_ALLOCS NUM_SLABS

// Test creation with invalid parameters
static int test_invalid_init() {
    SlabAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing invalid creation parameters...\n");
    #endif
    
    // Test with zero slab size
    assert(SlabAllocator_create(&allocator, 0, NUM_SLABS) == NULL);
    
    // Test with zero initial slabs
    assert(SlabAllocator_create(&allocator, SLAB_SIZE, 0) == NULL);
    
    // Test with NULL allocator
    assert(SlabAllocator_create(NULL, SLAB_SIZE, NUM_SLABS) == NULL);
    
    #ifdef VERBOSE
    printf("Invalid creation parameters test passed\n");
    #endif
    return 0;
}

// Test basic creation and destruction
static int test_create_destroy() {
    SlabAllocator allocator;
    
    #ifdef VERBOSE
    printf("Testing creation and destruction...\n");
    #endif
    
    // Test successful creation
    assert(SlabAllocator_create(&allocator, SLAB_SIZE, NUM_SLABS) != NULL);
    
    // Verify initial state
    assert(allocator.user_size == SLAB_SIZE);
    assert(allocator.free_list != NULL);
    assert(allocator.free_list_size == NUM_SLABS);
    assert(allocator.num_slabs == NUM_SLABS);
    
    // Clean up
    assert(SlabAllocator_destroy(&allocator) == 0);
    
    #ifdef VERBOSE
    printf("Creation and destruction test passed\n");
    #endif
    return 0;
}

// Test allocation and deallocation patterns
static int test_alloc_pattern() {
    SlabAllocator allocator;
    void* ptrs[NUM_SLABS];
    const unsigned char TEST_PATTERN = 0xAA;
    
    #ifdef VERBOSE
    printf("Testing allocation patterns...\n");
    #endif
    
    assert(SlabAllocator_create(&allocator, SLAB_SIZE, NUM_SLABS) != NULL);
    
    #ifdef VERBOSE
    printf("Allocating and filling with pattern...\n");
    #endif
    for (size_t i = 0; i < NUM_SLABS; i++) {
        ptrs[i] = SlabAllocator_malloc(&allocator);
        assert(ptrs[i] != NULL);
        fill_memory_pattern(ptrs[i], SLAB_SIZE, TEST_PATTERN);
    }
    
    #ifdef VERBOSE
    SlabAllocator_print_state(&allocator);
    printf("Verifying patterns...\n");
    #endif
    for (size_t i = 0; i < NUM_SLABS; i++) {
        assert(!verify_memory_pattern(ptrs[i], SLAB_SIZE, TEST_PATTERN));
    }
    
    #ifdef VERBOSE
    SlabAllocator_print_state(&allocator);
    printf("Releasing in reverse order...\n");
    #endif
    for (size_t i = NUM_SLABS; i > 0; i--) {
        assert(SlabAllocator_free(&allocator, ptrs[i-1]) == 0);
    }
    
    #ifdef VERBOSE
    SlabAllocator_print_state(&allocator);
    printf("Reallocating and verifying no pattern remains...\n");
    #endif
    for (size_t i = 0; i < NUM_SLABS; i++) {
        ptrs[i] = SlabAllocator_malloc(&allocator);
        assert(ptrs[i] != NULL);

        // Memory should be undefined, but we can write to it
        fill_memory_pattern(ptrs[i], SLAB_SIZE, ~TEST_PATTERN);
        assert(!verify_memory_pattern(ptrs[i], SLAB_SIZE, ~TEST_PATTERN));
    }
    
    #ifdef VERBOSE
    printf("Cleaning up...\n");
    #endif
    for (size_t i = 0; i < NUM_SLABS; i++) {
        assert(SlabAllocator_free(&allocator, ptrs[i]) == 0);
    }
    SlabAllocator_destroy(&allocator);
    
    #ifdef VERBOSE
    printf("Allocation patterns test passed\n");
    #endif
    return 0;
}

// Test allocation exhaustion
static int test_exhaustion() {
    SlabAllocator allocator;
    void* ptrs[NUM_SLABS + 1];  // One more than available
    
    #ifdef VERBOSE
    printf("Testing allocation exhaustion...\n");
    #endif
    
    assert(SlabAllocator_create(&allocator, SLAB_SIZE, NUM_SLABS) != NULL);
    
    #ifdef VERBOSE
    printf("Allocating all available slabs...\n");
    #endif
    for (size_t i = 0; i < NUM_SLABS; i++) {
        ptrs[i] = SlabAllocator_malloc(&allocator);
        assert(ptrs[i] != NULL);
    }
    
    #ifdef VERBOSE
    printf("Trying to allocate one more - should fail...\n");
    #endif
    assert(SlabAllocator_malloc(&allocator) == NULL);
    
    #ifdef VERBOSE
    printf("Freeing one and trying again - should succeed...\n");
    #endif
    assert(SlabAllocator_free(&allocator, ptrs[0]) == 0);
    ptrs[NUM_SLABS] = SlabAllocator_malloc(&allocator);
    assert(ptrs[NUM_SLABS] != NULL);
    
    #ifdef VERBOSE
    printf("Cleaning up...\n");
    #endif
    for (size_t i = 1; i < NUM_SLABS; i++) {
        SlabAllocator_free(&allocator, ptrs[i]);
    }
    assert(SlabAllocator_free(&allocator, ptrs[NUM_SLABS]) == 0);
    SlabAllocator_destroy(&allocator);
    
    #ifdef VERBOSE
    printf("Allocation exhaustion test passed\n");
    #endif
    return 0;
}

// Test invalid releases
static int test_invalid_free() {
    SlabAllocator allocator;
    void* ptr;
    
    #ifdef VERBOSE
    printf("Testing invalid releases...\n");
    #endif
    
    assert(SlabAllocator_create(&allocator, SLAB_SIZE, NUM_SLABS) != NULL);
    
    #ifdef VERBOSE
    printf("Allocating one slab...\n");
    #endif
    ptr = SlabAllocator_malloc(&allocator);
    assert(ptr != NULL);
    
    #ifdef VERBOSE
    printf("Testing NULL release...\n");
    #endif
    assert(SlabAllocator_free(&allocator, NULL) == -1);  // Should not crash
    
    #ifdef VERBOSE
    printf("Testing double release...\n");
    #endif
    assert(SlabAllocator_free(&allocator, ptr) == 0);
    assert(SlabAllocator_free(&allocator, ptr) == -1);  // Should not crash or corrupt
    
    #ifdef VERBOSE
    printf("Testing invalid pointer release...\n");
    #endif
    char invalid_ptr[SLAB_SIZE];
    assert(SlabAllocator_free(&allocator, invalid_ptr) == -1);  // Should not crash
    
    SlabAllocator_destroy(&allocator);
    
    #ifdef VERBOSE
    printf("Invalid releases test passed\n");
    #endif
    return 0;
}


int test_slab_allocator() {
    int result = 0;
    
    printf("=== Running SlabAllocator Tests ===\n");
    result |= test_invalid_init();
    result |= test_create_destroy();
    result |= test_alloc_pattern();
    result |= test_exhaustion();
    result |= test_invalid_free();

    if (result != 0) {
        printf(RED "Some SlabAllocator tests failed!\n" RESET);
    } else {
        printf(GREEN "All SlabAllocator tests passed!\n" RESET);
    }
    printf("=== SlabAllocator Tests Complete ===\n");
    
    return result;
}
