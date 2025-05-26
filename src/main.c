#include <main.h>

int main() {
    // Create a slab allocator with 64 byte slabs and 128 initial slabs
    SlabAllocator slab;
    if (!SlabAllocator_create(&slab, 64, 128)) {
        fprintf(stderr, "Failed to create slab allocator\n");
        return 1;
    }

    // Get the allocator interface
    Allocator* alloc = (Allocator*)&slab;

    // Test allocation
    void* ptr1 = alloc->malloc(alloc);
    if (!ptr1) {
        fprintf(stderr, "Failed to allocate memory\n");
        SlabAllocator_destructor(alloc);
        return 1;
    }

    // Test multiple allocations
    void* ptr2 = alloc->malloc(alloc);
    void* ptr3 = alloc->malloc(alloc);

    // Test freeing memory
    alloc->free(alloc, ptr1);
    alloc->free(alloc, ptr2);
    alloc->free(alloc, ptr3);

    // Clean up
    SlabAllocator_destructor(alloc);
    return 0;
}