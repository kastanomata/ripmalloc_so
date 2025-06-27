#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>  // For dirname()/basename()
#include <limits.h>  // For PATH_MAX
#include <string.h>
#include <dirent.h>
#include <ctype.h> 
#include <time.h>
#include <sys/resource.h>

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
int Allocator_benchmark_initialize(const char *file_name);

