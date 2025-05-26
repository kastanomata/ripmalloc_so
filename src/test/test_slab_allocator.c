#include <slab_allocator.h>
#include <test_time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef TIME
// Test functions that perform the actual allocations
static int test_standard_malloc_impl() {
    const size_t SLAB_SIZE = 1024;
    const size_t NUM_ALLOCS = 1000;
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
    }
    
    // Free blocks
    for (size_t i = 0; i < NUM_ALLOCS; i++) {
        free(ptrs[i]);
    }
    
    return 0;
}
#endif

static int test_slab_operations_impl() {
    SlabAllocator slab;
    const size_t NUM_SLABS = 1000;
    void* ptrs[NUM_SLABS];
    
    // Setup
    if (!SlabAllocator_create(&slab, 1024, NUM_SLABS)) {
        return -1;
    }
    
    // Allocate slabs
    for (size_t i = 0; i < NUM_SLABS; i++) {
        ptrs[i] = ((Allocator*)&slab)->malloc((Allocator*)&slab);
        if (!ptrs[i]) {
            for (size_t j = 0; j < i; j++) {
                ((Allocator*)&slab)->free((Allocator*)&slab, ptrs[j]);
            }
            SlabAllocator_destroy(&slab);
            return -1;
        }
    }
    
    // Free slabs
    for (size_t i = 0; i < NUM_SLABS; i++) {
        ((Allocator*)&slab)->free((Allocator*)&slab, ptrs[i]);
    }
    
    // Cleanup
    SlabAllocator_destroy(&slab);
    return 0;
}

#ifdef TIME
// Main test functions that handle timing
int test_standard_allocator() {
    printf("\n=== Standard Allocator Test ===\n");
    TimingResult timing = get_real_timing(test_standard_malloc_impl, "Standard malloc/free");
    return timing.real_time < 0 ? -1 : 0;
}
#endif

int test_slab_allocator() {
    int result = 0;
    
    // Always run slab allocator tests
    printf("\n=== Running Slab Allocator Tests ===\n");
    #ifdef TIME
    TimingResult slab_timing = get_real_timing(test_slab_operations_impl, "Slab allocator");
    result = slab_timing.real_time < 0 ? -1 : 0;
    
    // Only run standard allocator tests when TIME is defined
    printf("\n=== Running Standard Allocator Tests ===\n");
    int std_result = test_standard_allocator();
    result = result == 0 ? std_result : result;
    #else
    // In non-timing mode, just run the slab test directly
    result = test_slab_operations_impl();
    if (result != 0) {
        printf("All Slab allocator test failed\n");
    } else {
        printf("All Slab allocator test passed\n");
    }
    #endif
    
    return result;
}

