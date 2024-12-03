#include "../header/allocator.h"

Allocator* Allocator_init(uint32_t memory_size) {
    // memory for Allocator + memory that Allocator will manage
    uint8_t *memory = mmap(NULL, sizeof(Allocator) + memory_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    // todo error checking
    uint8_t *mapped_mem = memory + sizeof(Allocator);

    Allocator *a = (Allocator*) memory;
    a->managed_memory = mapped_mem;
    a->memory_size = memory_size;
    a->vtable.std.destructor = (Func_ptr) (Allocator_destroy);

    return a;
}

uint8_t Allocator_destroy(Allocator *a) {
    uint8_t *memory = a->managed_memory - sizeof(Allocator);
    int err = munmap(memory, a->memory_size + sizeof(Allocator));
    // todo error checking
    return 0;
};