#include "bitmap.h"

// Create a new bitmap with 'num_bits' capacity
Bitmap* bitmap_create(Bitmap *bitmap, int num_bits) {
    if (num_bits <= 0 || !bitmap) return NULL;

    bitmap->num_bits = num_bits;
    // Calculate words needed: ceil(num_bits / 32)
    bitmap->num_words = (num_bits + 31) / 32;
    bitmap->bits = (unsigned int*)calloc(bitmap->num_words, sizeof(unsigned int));
    
    if (!bitmap->bits) {
        return NULL;
    }
    return bitmap;
}

// Destroy bitmap and free memory
void bitmap_destroy(Bitmap *bitmap) {
    if (!bitmap) return;
    free(bitmap->bits);
}

// Set bit at given index (1-based)
void bitmap_set(Bitmap *bitmap, int index) {
    if (!bitmap || index < 0 || index >= bitmap->num_bits) return;
    
    int word_idx = index / 32;
    int bit_offset = index % 32;
    bitmap->bits[word_idx] |= (1u << bit_offset);
}

// Clear bit at given index (0-based)
void bitmap_clear(Bitmap *bitmap, int index) {
    if (!bitmap || index < 0 || index >= bitmap->num_bits) return;
    
    int word_idx = index / 32;
    int bit_offset = index % 32;
    bitmap->bits[word_idx] &= ~(1u << bit_offset);
}

// Check if bit at index is set
bool bitmap_test(Bitmap *bitmap, int index) {
    if (!bitmap || index < 0 || index >= bitmap->num_bits) return false;
    
    int word_idx = index / 32;
    int bit_offset = index % 32;
    return (bitmap->bits[word_idx] >> bit_offset) & 1u;
}

// Find index of first set bit (returns -1 if none)
int bitmap_find_first_set(Bitmap *bitmap) {
    if (!bitmap) return -1;
    
    for (int i = 0; i < bitmap->num_words; i++) {
        if (bitmap->bits[i] != 0) {
            // Find first set bit within the word
            for (int j = 0; j < 32; j++) {
                int bit_idx = i * 32 + j;
                if (bit_idx >= bitmap->num_bits) break;
                if (bitmap_test(bitmap, bit_idx)) 
                    return bit_idx;
            }
        }
    }
    return -1;
}

// Find index of first zero bit (returns -1 if none)
int bitmap_find_first_zero(Bitmap *bitmap) {
    if (!bitmap) return -1;
    
    for (int i = 0; i < bitmap->num_words; i++) {
        // Check if there are any zeros in this word
        if (bitmap->bits[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                int bit_idx = i * 32 + j;
                if (bit_idx >= bitmap->num_bits) break;
                if (!bitmap_test(bitmap, bit_idx)) 
                    return bit_idx;
            }
        }
    }
    return -1;
}

// Print bitmap info for debugging
int bitmap_print(Bitmap *bitmap) {
    if (!bitmap) return -1;
    
    printf("Bitmap Info:\n");
    printf("  Bits capacity: %d\n", bitmap->num_bits);
    printf("  Words allocated: %d\n", bitmap->num_words);
    printf("  Bit data: ");
    
    // Print bits as 0/1 string
    for (int i = 0; i < bitmap->num_bits; i++) {
        printf("%d", bitmap_test(bitmap, i) ? 1 : 0);
    }
    printf("\n");
    return 0;
}