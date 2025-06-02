#include <test_slab_allocator.h>
#define SLAB_SIZE 1024
#define NUM_SLABS 10
#define NUM_ALLOCS NUM_SLABS

// Test creation with invalid parameters
static int test_slab_creation_invalid() {
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
static int test_slab_create_destroy() {
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
    assert(allocator.free_list_size_max == NUM_SLABS);
    
    // Clean up
    SlabAllocator_destroy(&allocator);
    
    #ifdef VERBOSE
    printf("Creation and destruction test passed\n");
    #endif
    return 0;
}

// Test allocation and deallocation patterns
static int test_slab_alloc_pattern() {
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
        SlabAllocator_free(&allocator, ptrs[i-1]);
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
        SlabAllocator_free(&allocator, ptrs[i]);
    }
    SlabAllocator_destroy(&allocator);
    
    #ifdef VERBOSE
    printf("Allocation patterns test passed\n");
    #endif
    return 0;
}

// Test allocation exhaustion
static int test_slab_exhaustion() {
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
    SlabAllocator_free(&allocator, ptrs[0]);
    ptrs[NUM_SLABS] = SlabAllocator_malloc(&allocator);
    assert(ptrs[NUM_SLABS] != NULL);
    
    #ifdef VERBOSE
    printf("Cleaning up...\n");
    #endif
    for (size_t i = 1; i < NUM_SLABS; i++) {
        SlabAllocator_free(&allocator, ptrs[i]);
    }
    SlabAllocator_free(&allocator, ptrs[NUM_SLABS]);
    SlabAllocator_destroy(&allocator);
    
    #ifdef VERBOSE
    printf("Allocation exhaustion test passed\n");
    #endif
    return 0;
}

// Test invalid releases
static int test_slab_invalid_free() {
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
    SlabAllocator_free(&allocator, NULL);  // Should not crash
    
    #ifdef VERBOSE
    printf("Testing double release...\n");
    #endif
    SlabAllocator_free(&allocator, ptr);
    SlabAllocator_free(&allocator, ptr);  // Should not crash or corrupt
    
    #ifdef VERBOSE
    printf("Testing invalid pointer release...\n");
    #endif
    char invalid_ptr[SLAB_SIZE];
    SlabAllocator_free(&allocator, invalid_ptr);  // Should not crash
    
    SlabAllocator_destroy(&allocator);
    
    #ifdef VERBOSE
    printf("Invalid releases test passed\n");
    #endif
    return 0;
}

#ifdef TIME
// Test functions that perform the actual allocations
static int test_standard_malloc_impl() {

    void* ptrs[NUM_ALLOCS];
    
    // Allocate blocks
    for (size_t i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = malloc(SLAB_SIZE);
        if (!ptrs[i]) {
            // Cleanup previous allocations
            for (size_t j = 0; j < i; j++) {
                free(ptrs[j]);
            }
            return -1;
        }
        // Fill allocated memory with zeros
        memset(ptrs[i], 0, SLAB_SIZE);
    }
    
    // Free blocks
    for (size_t i = 0; i < NUM_ALLOCS; i++) {
        free(ptrs[i]);
    }
    
    return 0;
}
#endif
static int test_slab_operations_impl() {
    SlabAllocator allocator;
    void* ptrs[NUM_SLABS];
    
    // Setup
    if (!SlabAllocator_create(&allocator, SLAB_SIZE, NUM_SLABS)) {
        return -1;
    }
    
    // Allocate slabs and write to them
    for (size_t i = 0; i < NUM_SLABS ; i++) {
        ptrs[i] = SlabAllocator_malloc(&allocator);
        if (!ptrs[i]) {
            for (size_t j = 0; j < i; j++) {
                SlabAllocator_free(&allocator, ptrs[j]);
            }
            SlabAllocator_destroy(&allocator);
            return -1;
        }
        // Fill allocated memory with zeros
        memset(ptrs[i], 0, SLAB_SIZE);
    }

    // Free slabs
    for (size_t i = 0; i < NUM_SLABS; i++) {
        SlabAllocator_free(&allocator, ptrs[i]);
    }
    
    // Cleanup
    SlabAllocator_destroy(&allocator);
    return 0;
}

#ifdef TIME
// Main test functions that handle timing
int test_standard_allocator() {
    TimingResult timing = get_real_timing(test_standard_malloc_impl, "Standard malloc/free", NUM_RUNS);
    return timing.real_time < 0 ? -1 : 0;
}
#endif

int test_slab_allocator() {
    int result = 0;
    
    result |= test_slab_creation_invalid();
    result |= test_slab_create_destroy();
    result |= test_slab_alloc_pattern();
    result |= test_slab_exhaustion();
    result |= test_slab_invalid_free();
    
    #ifdef TIME
    printf("\n=== Running Timing Tests ===\n");
    TimingResult slab_timing = get_real_timing(test_slab_operations_impl, "Slab allocator", NUM_RUNS);
    result |= slab_timing.real_time < 0 ? -1 : 0;
    
    printf("\n=== Running Standard Allocator Tests ===\n");
    int std_result = test_standard_allocator();
    result |= std_result;
    #else

    printf("=== Running SlabAllocator Tests ===\n");
    result |= test_slab_operations_impl();
    #endif
    if (result != 0) {
        // Red color for failed tests
        printf("\033[1;31mSome SlabAllocator tests failed!\033[0m\n");
    } else {
        // Green color for passed tests
        printf("\033[1;32mAll SlabAllocator tests passed!\033[0m\n");
    }
    printf("=== SlabAllocator Tests Complete ===\n");
    
    return result;
}
