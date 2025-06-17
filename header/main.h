#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include <test/test_double_linked_list.h>
#include <test/test_bitmap.h>

#include <allocator.h>
// #include <test/test_allocator.h>
#include <slab_allocator.h>
#include <test/test_slab_allocator.h>
#include <buddy_allocator.h>
#include <test/test_buddy_allocator.h>
#include <bitmap_buddy_allocator.h>
#include <test/test_bitmap_buddy_allocator.h>

#include <helpers/freeform.h>
#include <helpers/benchmark.h>