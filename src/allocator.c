#include "../header/allocator.h"

// Initialize allocator with specified memory size
Allocator* Allocator_init(uint32_t memory_size) {
    if (memory_size == 0) {
        fprintf(stderr, "Error: memory_size cannot be 0\n");
        return NULL;
    }

    // memory for Allocator + memory that Allocator will manage
    uint8_t *memory = mmap(NULL, sizeof(Allocator) + memory_size, 
                        PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }

    uint8_t *mapped_mem = memory + sizeof(Allocator);

    Allocator *a = (Allocator*) memory;
    a->managed_memory = mapped_mem;
    a->memory_size = memory_size;
    a->vtable.std.destructor = (Func_ptr) (Allocator_destroy);

    return a;
}

// Destroy allocator and free all resources
uint8_t Allocator_destroy(Allocator *a) {
    if (a == NULL) {
        fprintf(stderr, "Error: allocator is NULL\n");
        return -1;
    }

    uint8_t *memory = a->managed_memory - sizeof(Allocator);
    int err = munmap(memory, a->memory_size + sizeof(Allocator));
    if (err != 0) {
        perror("munmap failed");
        return err;
    }
    return 0;
}