#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    unsigned int *bits;   // Array holding bitmap data
    int num_bits;         // Total bits in the bitmap
    int num_words;        // Number of words allocated
} Bitmap;

Bitmap* bitmap_create(Bitmap *bitmap, int num_bits);
void bitmap_destroy(Bitmap *bitmap);
void bitmap_set(Bitmap *bitmap, int index);
void bitmap_clear(Bitmap *bitmap, int index);
bool bitmap_test(Bitmap *bitmap, int index);
int bitmap_find_first_set(Bitmap *bitmap);
int bitmap_find_first_zero(Bitmap *bitmap);
int bitmap_print(Bitmap *bitmap);