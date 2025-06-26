#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slab_allocator.h>
#include <buddy_allocator.h>
#include <bitmap_buddy_allocator.h>


#define RED     "\x1B[31m"
#define RESET   "\x1B[0m"

#define PAGESIZE sysconf(_SC_PAGESIZE)
#define DEF_LEVELS_NUMBER 4
#define MAX_LEVELS 32


int freeform();