#include "../header/allocator.h"

uint8_t Allocator_init(Allocator* a, uint32_t memory_size) {
    if (memory_size == 0) {
        fprintf(stderr, "Error: memory_size cannot be 0\n");
        return -1;
    }

    uint8_t *memory = mmap(NULL, memory_size, 
                        PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    a->managed_memory = memory;
    a->memory_size = memory_size;
    a->vtable.constructor = Allocator_init;
    a->vtable.destructor = Allocator_destroy;

    return 0;
}

uint8_t Allocator_destroy(Allocator *a) {
    if (a == NULL) {
        fprintf(stderr, "Error: allocator is NULL\n");
        return -1;
    }

    int err = munmap(a->managed_memory, a->memory_size);
    if (err != 0) {
        perror("munmap failed");
        return err;
    }
    return 0;
}