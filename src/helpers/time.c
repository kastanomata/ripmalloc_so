#include <helpers/time.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

static long timeval_to_micros(struct timeval *tv) {
    return (tv->tv_sec * 1000000) + tv->tv_usec;
}

TimingResult get_real_timing(TestFunction test_fn, const char* test_name, int num_runs) {
    TimingResult result = {0, 0, 0};
    TimingResult total = {0, 0, 0};
    struct rusage start_usage, end_usage;
    struct timeval start_wall, end_wall;
    int failed_runs = 0;
    
    // Run test num_runs times
    for (int i = 0; i < num_runs; i++) {
        // Get initial timestamps
        getrusage(RUSAGE_SELF, &start_usage);
        gettimeofday(&start_wall, NULL);
        
        // Run the test
        int test_result = test_fn();
        if (test_result != 0) {
            failed_runs++;
        }
        
        // Get end timestamps
        getrusage(RUSAGE_SELF, &end_usage);
        gettimeofday(&end_wall, NULL);
        
        // Calculate times for this run
        result.user_time = 
            timeval_to_micros(&end_usage.ru_utime) - 
            timeval_to_micros(&start_usage.ru_utime);
        
        result.system_time = 
            timeval_to_micros(&end_usage.ru_stime) - 
            timeval_to_micros(&start_usage.ru_stime);
        
        result.real_time = 
            timeval_to_micros(&end_wall) - 
            timeval_to_micros(&start_wall);
            
        // Add to totals
        total.user_time += result.user_time;
        total.system_time += result.system_time;
        total.real_time += result.real_time;
    }
    
    // Calculate averages
    result.user_time = total.user_time / num_runs;
    result.system_time = total.system_time / num_runs;
    result.real_time = total.real_time / num_runs;
    
    printf("\nTiming results for %s (average of %d runs):\n", test_name, num_runs);
    printf("  User time:   %ld microseconds\n", result.user_time);
    printf("  System time: %ld microseconds\n", result.system_time);
    printf("  Real time:   %ld microseconds\n", result.real_time);
    printf("  CPU time:    %ld microseconds\n", result.user_time + result.system_time);
    if (failed_runs > 0) {
        printf("  Warning: %d out of %d test runs returned non-zero status\n", failed_runs, num_runs);
    }
    
    return result;
}
