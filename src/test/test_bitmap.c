#include <test/test_bitmap.h>

int test_bitmap_create_destroy() {
    // Test creation with valid size
    Bitmap b;
    Bitmap *r = bitmap_create(&b, 64);
    assert(r != NULL);
    assert(b.num_bits == 64);
    assert(b.num_words == 2);  // 64 bits / 32 = 2 words
    bitmap_destroy(&b);
    // Note: We can't directly verify destruction, but valgrind can check for leaks
    r = bitmap_create(&b, -1); // Invalid size
    assert(r == NULL); // Should return NULL for invalid size
    
    return 0;
}

int test_bitmap_set_clear_test() {
    Bitmap b;
    bitmap_create(&b, 128);
    assert(&b != NULL);
    
    // Test setting and testing bits
    bitmap_set(&b, 0);
    bitmap_set(&b, 31);
    bitmap_set(&b, 32);
    bitmap_set(&b, 100);
    
    assert(bitmap_test(&b, 0) == true);
    assert(bitmap_test(&b, 31) == true);
    assert(bitmap_test(&b, 32) == true);
    assert(bitmap_test(&b, 100) == true);
    assert(bitmap_test(&b, 1) == false);   // Unset bit
    assert(bitmap_test(&b, 127) == false); // Unset bit

    // Test clearing bits
    bitmap_clear(&b, 31);
    bitmap_clear(&b, 100);

    assert(bitmap_test(&b, 31) == false);
    assert(bitmap_test(&b, 100) == false);
    assert(bitmap_test(&b, 0) == true);    // Should remain set

    // Test boundary conditions
    bitmap_set(&b, 127);  // Last bit
    assert(bitmap_test(&b, 127) == true);

    // Test invalid indices
    bitmap_set(&b, 128);  // Should be ignored
    bitmap_clear(&b, -1); // Should be ignored
    assert(bitmap_test(&b, 128) == false);

    bitmap_destroy(&b);
    return 0;
}

int test_bitmap_find_first() {
    Bitmap b;
    bitmap_create(&b, 96);  // 3 words
    assert(&b != NULL);

    // Test empty bitmap
    assert(bitmap_find_first_set(&b) == -1);
    assert(bitmap_find_first_zero(&b) == 0);

    // Set some bits
    bitmap_set(&b, 0);
    bitmap_set(&b, 32);
    bitmap_set(&b, 64);

    // Test find first set
    assert(bitmap_find_first_set(&b) == 0);
    bitmap_clear(&b, 0);
    assert(bitmap_find_first_set(&b) == 32);
    bitmap_clear(&b, 32);
    assert(bitmap_find_first_set(&b) == 64);
    bitmap_clear(&b, 64);
    // Test find first zero
    bitmap_set(&b, 0);   // Set bit 0 again
    assert(bitmap_find_first_zero(&b) == 1);  // First zero after bit 0

    // Fill first word completely
    for (int i = 0; i < 32; i++) {
        bitmap_set(&b, i);
    }
    assert(bitmap_find_first_zero(&b) == 32);  // Should skip full first word

    // Test completely full bitmap
    for (int i = 0; i < 96; i++) {
        bitmap_set(&b, i);
    }
    assert(bitmap_find_first_zero(&b) == -1);

    bitmap_destroy(&b);
    return 0;
}

int test_bitmap_print() {
    // Mostly for visual inspection
    Bitmap b;
    bitmap_create(&b, 16);  // 16 bits
    assert(&b != NULL);
    bitmap_set(&b, 0);
    bitmap_set(&b, 2);
    bitmap_set(&b, 15);

    #ifdef VERBOSE
    printf("Expected bitmap pattern: 101000000000001\n");
    printf("Actual output:\n");
    bitmap_print(&b);
    #endif

    bitmap_destroy(&b);
    return 0;
}

int test_bitmap() {
    printf("=== Running Bitmap Tests ===\n");

    int tests_passed = 0;
    int total_tests = 4;
    
    if (test_bitmap_create_destroy() == 0) tests_passed++;
    if (test_bitmap_set_clear_test() == 0) tests_passed++;
    if (test_bitmap_find_first() == 0) tests_passed++;
    if (test_bitmap_print() == 0) tests_passed++;
    if (tests_passed == total_tests) {
        printf("\033[1;32mAll Bitmap tests passed!\033[0m\n");
    } else {
        printf("\033[1;31mSome Bitmap tests failed!\033[0m\n");
    }
    printf("=== Bitmap Tests Complete ===\n");
    return tests_passed == total_tests ? 0 : 1;
}