#include <buddy_allocator.h>
#include <helpers/memory_manipulation.h>
#include <unistd.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_buddy_allocator();

#define TOTAL_SIZE (1 << 10)  // 1KB
#define NUM_LEVELS 4          // Results in 128B min block size (1024/2^4)

