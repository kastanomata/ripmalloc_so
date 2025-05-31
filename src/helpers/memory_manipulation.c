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