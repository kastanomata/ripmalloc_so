// header/allocator.h
#define _GNU_SOURCE
#include "commons.h"
#pragma once

// First forward declare the struct
typedef struct Allocator Allocator;

// Define function pointer types
typedef uint8_t (*ConstructorFunc)(Allocator*, uint32_t);
typedef uint8_t (*DestructorFunc)(Allocator*);

// Vtable structure
typedef struct {
    ConstructorFunc constructor; // TODO I want to be able to call with this pointer Allocator_init after doing Allocator a; 
    DestructorFunc destructor;
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