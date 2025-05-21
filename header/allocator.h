// allocator.h
#define _GNU_SOURCE
#include "commons.h"
#pragma once

// First forward declare the struct
typedef struct Allocator Allocator;

// Then define the function pointer type using the forward declared type
typedef uint8_t (*DestructorFunc)(Allocator*);

// Now define the Vtable structure
typedef struct {
    DestructorFunc destructor;
} Vtable;

// Finally define the Allocator structure
struct Allocator {
    Vtable vtable;
    uint8_t *managed_memory;
    uint32_t memory_size;
};

// Initialize allocator with specified memory size
Allocator* Allocator_init(uint32_t memory_size);

// Destroy allocator and free all resources
uint8_t Allocator_destroy(Allocator *a);