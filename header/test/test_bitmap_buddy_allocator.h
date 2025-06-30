#pragma once
#include <assert.h>
#include <bitmap_buddy_allocator.h>
#include <allocator.h>
#include <buddy_allocator.h>
#include <memory_manipulation.h>

int test_bitmap_buddy_allocator();

#define MEMORY_SIZE (1 << 10)  // 1KB
#define NUM_LEVELS 4          // Results in 128B min block size (1024/2^4)