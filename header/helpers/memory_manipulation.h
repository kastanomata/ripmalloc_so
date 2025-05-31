#pragma once
#include <stdio.h>
#include <stddef.h>
#include <string.h>

void fill_memory_pattern(void* ptr, size_t size, unsigned char pattern);
int verify_memory_pattern(void* ptr, size_t size, unsigned char pattern);