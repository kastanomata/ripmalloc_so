#include <stdio.h>
#include "../header/commons.h"
#include "../header/allocator.h"

// TODO: move test suite to test.c. Continue to write tests when you add features.
// Test program for allocator functionality
int main() {
    printf("Allocator Test Program\n");
    
    // Test 1: Basic allocation
    int size = 4080;
    printf("Test 1: Creating allocator with %d bytes...\n", size);
    Allocator *a = Allocator_init(size);
    if (a == NULL) {
        fprintf(stderr, "Test 1 failed: allocator creation failed\n");
        return 1;
    }
    printf("Test 1 passed: Allocator created successfully\n");
    
    // Test 2: Verify allocator information
    printf("\nTest 2: Verifying allocator information...\n");
    if (a->memory_size != size) {
        fprintf(stderr, "Test 2 failed: memory_size mismatch (%d != %d)\n", 
                a->memory_size, size);
        Allocator_destroy(a);
        return 1;
    }
    printf("Test 2 passed: Allocator information correct\n");
    
    // Test 3: Destroy allocator
    printf("\nTest 3: Destroying allocator...\n");
    int ret = Allocator_destroy(a);
    if (ret != 0) {
        fprintf(stderr, "Test 3 failed: allocator destruction failed with code %d\n", ret);
        return 1;
    }
    printf("Test 3 passed: Allocator destroyed successfully\n");
    
    // Test 4: Invalid size test
    printf("\nTest 4: Testing invalid size (0 bytes)...\n");
    Allocator *invalid = Allocator_init(0);
    if (invalid != NULL) {
        fprintf(stderr, "Test 4 failed: allocator should not be created with size 0\n");
        Allocator_destroy(invalid);
        return 1;
    }
    printf("Test 4 passed: Allocator correctly rejected invalid size\n");
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}