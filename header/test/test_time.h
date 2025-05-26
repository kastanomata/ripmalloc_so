#pragma once

// Function pointer type for allocation test functions
typedef int (*TestFunction)();

// Structure to hold timing results
typedef struct {
    long user_time;    // Time spent in user mode (microseconds)
    long system_time;  // Time spent in kernel mode (microseconds)
    long real_time;    // Wall clock time (microseconds)
} TimingResult;

// Get timing measurements for a test function
TimingResult get_real_timing(TestFunction test_fn, const char* test_name, int num_runs);

// Start timing both program and system time
void start_timing();

// Stop timing and print results 
void stop_timing();

