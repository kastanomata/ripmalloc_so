#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdarg.h>

// ANSI color codes
#define RED     "\x1B[31m"
#define RESET   "\x1B[0m"

// Forward declaration
typedef struct Allocator Allocator;

// Define function pointer types
typedef void* (*InitFunc)(Allocator*, ...);
typedef void* (*DestructorFunc)(Allocator*, ...);
typedef void* (*MallocFunc)(Allocator*, ...);  
typedef void* (*FreeFunc)(Allocator*, ...);       

// Allocator structure
struct Allocator {
    InitFunc init; 
    DestructorFunc dest;
    MallocFunc malloc;
    FreeFunc free; 
};
