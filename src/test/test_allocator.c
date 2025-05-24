#include "../header/test/test_allocator.h"

int test_allocator_init() {
    Allocator a;  // Stack-allocated allocator
    int ret = a.vtable.constructor(&a, 1024);
    if (ret != 0) {
        fprintf(stderr, "Allocator initialization failed\n");
        return -1;
    }
    if (a.memory_size != 1024) {
        fprintf(stderr, "Allocator memory size mismatch\n");
        return -1;
    }
    if (a.managed_memory == NULL) {
        fprintf(stderr, "Allocator managed memory is NULL\n");
        return -1;
    }
    
    // Clean up
    if (a.vtable.destructor(&a) != 0) {
        fprintf(stderr, "Cleanup failed after init test\n");
    }
    return 0;
}

int test_allocator_destroy() {
    Allocator a;
    if (a.vtable.constructor(&a, 1024) != 0) {
        fprintf(stderr, "Allocator initialization failed\n");
        return -1;
    }
    if (a.vtable.destructor(&a) != 0) {
        fprintf(stderr, "Allocator destruction failed\n");
        return -1;
    }
    return 0;
}

int test_allocator_invalid_size() {
    Allocator a;
    int ret = a.vtable.constructor(&a, 1024);
    if (ret == 0) {  // Should fail
        fprintf(stderr, "Allocator should not be created with size 0\n");
        a.vtable.destructor(&a);
        return -1;
    }
    return 0;
}

int test_allocator() {
    printf("Allocator Class Test Program\n");
    printf("Running tests...\n");
    int c = 3;
    if (test_allocator_init() != 0) {
        fprintf(stderr, "Allocator initialization test failed\n");
        c--;
    }
    if (test_allocator_destroy() != 0) {
        fprintf(stderr, "Allocator destruction test failed\n");
        c--;
    }
    if (test_allocator_invalid_size() != 0) {
        fprintf(stderr, "Allocator invalid size test failed\n");
        c--;
    }
    printf("%d/%d tests passed\n", c, 3);
    return c;
}