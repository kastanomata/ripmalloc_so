#include <slab_allocator.h>
#include <assert.h>
#include <stdio.h>

int test_slab_create() {
    SlabAllocator slab;
    SlabAllocator* created = SlabAllocator_create(&slab, 1024, 10);
    assert(created == &slab);
    assert(slab.slab_size == 1024);
    assert(slab.free_list_size == 10);
    assert(slab.managed_memory != NULL);
    SlabAllocator_destroy(&slab);
    return 0;
}

int test_slab_alloc_free() {
    SlabAllocator slab;
    SlabAllocator_create(&slab, 1024, 10);
    
    // Allocate some slabs
    void* ptr1 = ((Allocator*)&slab)->malloc((Allocator*)&slab);
    void* ptr2 = ((Allocator*)&slab)->malloc((Allocator*)&slab);
    void* ptr3 = ((Allocator*)&slab)->malloc((Allocator*)&slab);
    
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(ptr3 != NULL);
    assert(slab.free_list_size == 7);
    
    // Free them
    void* result = ((Allocator*)&slab)->free((Allocator*)&slab, ptr1);
    assert(result != NULL);
    assert(slab.free_list_size == 8);
    
    result = ((Allocator*)&slab)->free((Allocator*)&slab, ptr2);
    assert(result != NULL);
    assert(slab.free_list_size == 9);
    
    result = ((Allocator*)&slab)->free((Allocator*)&slab, ptr3);
    assert(result != NULL);
    assert(slab.free_list_size == 10);
    
    SlabAllocator_destroy(&slab);
    return 0;
}

int test_slab_allocator() {
    printf("Slab Allocator Test Program\n");
    printf("Running tests...\n");
    
    int tests_passed = 0;
    int total_tests = 2;
    
    if (test_slab_create() == 0) {
        printf("test_slab_create: PASSED\n");
        tests_passed++;
    } else {
        printf("test_slab_create: FAILED\n");
    }
    
    if (test_slab_alloc_free() == 0) {
        printf("test_slab_alloc_free: PASSED\n");
        tests_passed++;
    } else {
        printf("test_slab_alloc_free: FAILED\n");
    }
    
    printf("\nTest Results: %d/%d tests passed\n", tests_passed, total_tests);
    return tests_passed == total_tests ? 0 : -1;
}
