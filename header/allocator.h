// allocator.h
#define _GNU_SOURCE // for MAP_ANON
#include "commons.h"
#pragma once

// Function pointer type for virtual table methods
typedef void* Func_ptr;

// Virtual table structure containing constructor and destructor pointers
typedef struct {
    Func_ptr constructor;
    Func_ptr destructor;
} Vtable;

// Allocator's virtual table structure
typedef struct {
    Vtable std;
} AllocatorVtable;

// Main allocator structure
typedef struct {
    AllocatorVtable vtable;  // Virtual table
    uint8_t * managed_memory; // Pointer to managed memory block
    uint32_t memory_size;     // Size of managed memory block
} Allocator;

// Initialize allocator with specified memory size
Allocator* Allocator_init(uint32_t memory_size);

// Destroy allocator and free all resources
uint8_t Allocator_destroy(Allocator *a);