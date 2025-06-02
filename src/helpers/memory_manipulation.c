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
    for (size_t i = 0; i < size; i++) {
        printf("0x%02x ", bytes[i]);
        bytes_printed++;
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (size % 16 != 0) {
        printf("\n");
    }
    printf("Total bytes printed: %zu\n", bytes_printed);
    return 0;
}