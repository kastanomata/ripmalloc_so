#pragma once
#include <bitmap_buddy_allocator.h>
#include <allocator.h>
#include <buddy_allocator.h>
#include <memory_manipulation.h>

#define PAGESIZE sysconf(_SC_PAGESIZE)
#define DEF_LEVELS_NUMBER 4

#define line printf("- - - - - - - - - - - - - - - - - - - - - - - -\n");

int test_bitmap_buddy_allocator();