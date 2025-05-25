#pragma once
#define _GNU_SOURCE
#include "commons.h"
// First forward declare the struct
typedef struct Allocator Allocator;

// Define function pointer types
typedef uint8_t (*ConstructorFunc)(Allocator*, uint32_t);
typedef uint8_t (*DestructorFunc)(Allocator*);
typedef uint8_t (*AllocatorFunc)(Allocator*, size_t);  
typedef uint8_t (*FreeFunc)(Allocator*, void*);       

// Vtable structure
typedef struct {
    ConstructorFunc constructor; 
    DestructorFunc destructor;
    AllocatorFunc malloc;
    FreeFunc free; 
} Vtable;

// Allocator structure
struct Allocator {
    Vtable vtable;
    uint8_t *managed_memory;
    uint32_t memory_size;
};

// Initialize allocator with specified memory size
uint8_t Allocator_init(Allocator* a, uint32_t memory_size);

// Destroy allocator and free all resources
uint8_t Allocator_destroy(Allocator *a);

// This calls mmap for requests that are bigger than specified maybe?
uint8_t Allocator_malloc(Allocator *a, size_t size);
uint8_t Allocator_free(Allocator *a, void* ptr);

// Helper functions