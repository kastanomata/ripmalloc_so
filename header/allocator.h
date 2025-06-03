#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdarg.h>

#define RED     "\x1B[31m"
#define RESET   "\x1B[0m"

#define PAGESIZE sysconf(_SC_PAGESIZE)
#define DEF_LEVELS_NUMBER 4

#define wait_user() scanf("%*c", 1, NULL)

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
