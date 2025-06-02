#pragma once
#include <slab_allocator.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <helpers/memory_manipulation.h>
#ifdef TIME
#include <helpers/time.h>
#define NUM_RUNS 1
#endif



typedef struct {
    int passed;
    long micros;
} TestResult;

TestResult test_standard_reserve();
TestResult test_slab_operations();
int test_standard_allocator();
int test_slab_allocator();

