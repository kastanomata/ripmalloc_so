#include "../header/test/test_allocator.h"

int test_allocator_init() {
    Allocator *a = Allocator_init(1024);
    if (a == NULL) {
        fprintf(stderr, "Allocator initialization failed\n");
        return -1;
    }
    if (a->memory_size != 1024) {
        fprintf(stderr, "Allocator memory size mismatch\n");
        return -1;
    }
    if (a->managed_memory == NULL) {
        fprintf(stderr, "Allocator managed memory is NULL\n");
        return -1;
    }
    return 0;
}

int test_allocator_destroy() {
    Allocator *a = Allocator_init(1024);
    if (a == NULL) {
        fprintf(stderr, "Allocator initialization failed\n");
        return -1;
    }
    if (Allocator_destroy(a) != 0) {
        fprintf(stderr, "Allocator destruction failed\n");
        return -1;
    }
    return 0;
}

int test_allocator_invalid_size() {
    Allocator *a = Allocator_init(0);
    if (a != NULL) {
        fprintf(stderr, "Allocator should not be created with size 0\n");
        Allocator_destroy(a);
        return -1;
    }
    return 0;
}

int test_allocator() {
    printf("Allocator Class Test Program\n");
    printf("Running tests...\n");
    if (test_allocator_init() != 0) {
        fprintf(stderr, "Allocator initialization test failed\n");
        return -1;
    }
    if (test_allocator_destroy() != 0) {
        fprintf(stderr, "Allocator destruction test failed\n");
        return -1;
    }
    if (test_allocator_invalid_size() != 0) {
        fprintf(stderr, "Allocator invalid size test failed\n");
        return -1;
    }
    printf("All tests passed!\n");
    return 0;
}