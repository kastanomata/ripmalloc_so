#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdarg.h>

// First forward declare the struct
typedef struct Allocator Allocator;

// Define function pointer types
typedef uint8_t (*ConstructorFunc)(Allocator*, ...);
typedef uint8_t (*DestructorFunc)(Allocator*, ...);
typedef uint8_t (*AllocatorFunc)(Allocator*, ...);  
typedef uint8_t (*FreeFunc)(Allocator*, ...);       

// Allocator structure
struct Allocator {
    ConstructorFunc constructor; 
    DestructorFunc destructor;
    AllocatorFunc malloc;
    FreeFunc free; 
};
