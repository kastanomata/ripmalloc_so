#define _GNU_SOURCE // for MAP_ANON
#include "commons.h"

typedef void* Func_ptr;

typedef struct {
    Func_ptr constructor;
    Func_ptr destructor;
} Vtable;

typedef struct {
    Vtable std;
} AllocatorVtable;

typedef struct {
    AllocatorVtable vtable;
    uint8_t * managed_memory;
    uint32_t memory_size;

} Allocator;

Allocator* Allocator_init(uint32_t memory_size);

uint8_t Allocator_destroy(Allocator *a);