#include <helpers/memory_manipulation.h>

// Helper function to fill memory with pattern
void fill_memory_pattern(void* ptr, size_t size, unsigned char pattern) {
    memset(ptr, pattern, size);
}

// Helper function to verify memory pattern
int verify_memory_pattern(void* ptr, size_t size, unsigned char pattern) {
    unsigned char* bytes = (unsigned char*)ptr;
    for (size_t i = 0; i < size; i++) {
        if (bytes[i] != pattern) {
            printf("Memory corruption at offset %zu: expected 0x%02x, got 0x%02x\n", 
                   i, pattern, bytes[i]);
            return -1;
        }
    }
    return 0;
}

int print_memory_pattern(void* ptr, size_t size) {
    unsigned char* bytes = (unsigned char*)ptr;
    printf("Memory pattern at %p:\n", ptr);
    size_t bytes_printed = 0;
    size_t row_width = 8; // 64 bits = 8 bytes

    unsigned char prev_row[8] = {0};
    int repeat_count = 0;
    int first_row = 1;

    for (size_t i = 0; i < size; i += row_width) {
        size_t this_row = (i + row_width <= size) ? row_width : (size - i);

        // Compare with previous row
        int same_as_prev = 0;
        if (!first_row && this_row == row_width && memcmp(prev_row, &bytes[i], row_width) == 0) {
            same_as_prev = 1;
        }

        if (same_as_prev) {
            repeat_count++;
        } else {
            if (repeat_count >= 3) {
                printf("... previous row repeated for %zu bytes ...\n", repeat_count * row_width);
            }
            // Print the row
            printf("%04zx: ", i);
            for (size_t j = 0; j < this_row; j++) {
                printf("0x%02x ", bytes[i + j]);
            }
            printf("\n");
            repeat_count = 1;
            memcpy(prev_row, &bytes[i], this_row);
            first_row = 0;
        }
        bytes_printed += this_row;
    }
    if (repeat_count >= 3) {
        printf("... previous row repeated for %zu bytes ...\n", repeat_count * row_width);
    }
    printf("Total bytes printed: %zu\n", bytes_printed);
    return 0;
}