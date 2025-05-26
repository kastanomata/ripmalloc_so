#pragma once

typedef struct {
    int passed;
    long micros;
} TestResult;

TestResult test_standard_malloc();
TestResult test_slab_operations();
int test_standard_allocator();
int test_slab_allocator();

