#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>  // For dirname()/basename()
#include <limits.h>  // For PATH_MAX
#include <string.h>
#include <dirent.h>

#include <allocator.h>
#include <slab_allocator.h>
#include <buddy_allocator.h>
#include <bitmap_buddy_allocator.h>

#include <helpers/parse.h>

#define PROJECT_FOLDER "."
#define BENCHMARK_FOLDER "./benchmarks"

union GeneralAllocator {
  SlabAllocator slab;
  BuddyAllocator buddy;
  BitmapBuddyAllocator bitmap;
};

int benchmark();
int run_benchmark_from_file(const char *file_name);

