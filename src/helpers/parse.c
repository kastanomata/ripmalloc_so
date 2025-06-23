#include <parse.h>

enum AllocatorType parse_allocator_create(FILE *file) {
  char line[256];
  // Loop until we find a line that does not start with '%'
  while (fgets(line, sizeof(line), file)) {
    if (line[0] != '%') {
      printf("Allocator creation command: %s\n", line);
      break;
    }
  }
  // the structure of the line is:
  // i,<allocator_type>,<param1>,<param2>,...
  char *token = strtok(line, ",");
  if (!token) {
    #ifdef DEBUG
    fprintf(stderr, RED "Invalid allocator creation command format: %s\n" RESET, line);
    #endif
    return -1;
  }
  if (strcmp(token, "i") != 0) {
    #ifdef DEBUG
    fprintf(stderr, RED "Invalid allocator creation command prefix: %s\n" RESET, token);
    #endif
    return -1;
  }
  token = strtok(NULL, ",");
  if (!token) {
    #ifdef DEBUG
    fprintf(stderr, RED "No allocator type specified in command\n" RESET);
    #endif
    return -1;
  }
  // Remove trailing newline from token if present
  token[strcspn(token, "\r\n")] = 0;
  enum AllocatorType type = -1;  // Default to an invalid type
  if (strcmp(token, "slab") == 0) {
    type = SLAB_ALLOCATOR;
  } else if (strcmp(token, "buddy") == 0) {
    type = BUDDY_ALLOCATOR;
  } else if (strcmp(token, "bitmap") == 0) {
    type = BITMAP_BUDDY_ALLOCATOR;
  } else {
    #ifdef DEBUG
    fprintf(stderr, RED "Unknown allocator type: '%s'\n" RESET, token);
    #endif
  }
  return type;
}

union AllocatorConfigData parse_allocator_create_parameters(FILE *file, struct AllocatorConfig *config) {
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    if (line[0] != '%') 
    break;  
  }
  // the structure of the line is:
  // p,<param1>,<param2>,...
  
  char *token = strtok(line, ",");
  if (!token || strcmp(token, "p") != 0) {
    #ifdef DEBUG
    fprintf(stderr, RED "Invalid allocator parameters format: %s\n" RESET, line);
    #endif
  }
  union AllocatorConfigData data = {0}; 
  if (config->type == SLAB_ALLOCATOR) {
    token = strtok(NULL, ",");
    if (!token) {
      #ifdef DEBUG
      fprintf(stderr, RED "No slab size specified\n" RESET);
      #endif
      return data; 
    }
    data.slab.slab_size = strtoul(token, NULL, 10);
    
    token = strtok(NULL, ",");
    if (!token) {
      #ifdef DEBUG
      fprintf(stderr, RED "No number of slabs specified\n" RESET);
      #endif
      return data; 
    }
    data.slab.n_slabs = strtoul(token, NULL, 10);
    
  } else if (config->type == BUDDY_ALLOCATOR || config->type == BITMAP_BUDDY_ALLOCATOR) {
    // Parse buddy allocator parameters
    token = strtok(NULL, ",");
    if (!token) {
      #ifdef DEBUG
      fprintf(stderr, RED "No total size specified for buddy allocator\n" RESET);
      #endif
      return data;
    }
    data.buddy.total_size = strtoul(token, NULL, 10);
    
    token = strtok(NULL, ",");
    if (!token) {
      #ifdef DEBUG
      fprintf(stderr, RED "No max levels specified for buddy allocator\n" RESET);
      #endif
      return data;
    }
    data.buddy.max_levels = strtoul(token, NULL, 10);
    
  } else {
    #ifdef DEBUG
    fprintf(stderr, RED "Unknown allocator type for parameters: %d\n" RESET, config->type);
    #endif
  }
  
  return data;
}

int parse_allocator_request(const char *line, struct AllocatorConfig *config, char **pointers, int num_pointers, long *allocation_counter) {
  // the structure of the line is:
  // for variable size allocation:
  // a,<index>,<size>
  // for fixed size allocation:
  // a,<index>
  Allocator *allocator = config->allocator;
  char *token = strtok((char *)line, ",");
  enum RequestType request_type = ALLOCATE;
  if (strcmp(token, "a") == 0) {
    request_type = ALLOCATE;
  } else if (strcmp(token, "f") == 0) {
    request_type = FREE;
  } else {
    #ifdef DEBUG
    printf(RED "Invalid request type: %s\n" RESET, token);
    #endif
    return -1;
  }
  token = strtok(NULL, ",");
  if (!token) {
    #ifdef DEBUG
    printf(RED "No index specified in allocator request\n" RESET);
    #endif
    return -1;
  }
  int index = atoi(token);
  if (index < 0 || index >= num_pointers) {
    #ifdef DEBUG
    printf(RED "Invalid index specified in allocator request: %d\n" RESET, index);
    #endif
    return -1;
  }

  switch (request_type) {
    case ALLOCATE: {
      if (pointers[index] != NULL) {
        #ifdef DEBUG
        printf(RED "Pointer at index %d already allocated: %p\n" RESET, index, pointers[index]);
        #endif
        return -1;  // Pointer already allocated
      }
      if (config->is_variable_size_allocation == false) {
        // Fixed size allocation
        pointers[index] = allocator->malloc(allocator);
        if (pointers[index] == NULL) {
          #ifdef DEBUG
          printf(RED "Failed to allocate memory for pointer at index %d\n" RESET, index);
          #endif
          return -1;
        }
        #ifdef VERBOSE
        printf(GREEN "Pointer at index %d allocated successfully: %p\n" RESET, index, pointers[index]);
        #endif
        (*allocation_counter)++;
      } else {
        // Variable size allocation
        token = strtok(NULL, ",");
        if (!token) {
          #ifdef DEBUG
          printf(RED "No size specified for variable size allocation\n" RESET);
          #endif
          return -1;
        }
        int size = atoi(token);
        if (size <= 0) {
          #ifdef DEBUG
          printf(RED "Invalid size specified for variable size allocation: %d\n" RESET, size);
          #endif
          return -1;
        }
        pointers[index] = allocator->malloc(allocator, size);
        if (pointers[index] == NULL) {
          #ifdef DEBUG
          printf(RED "Failed to allocate memory for pointer at index %d\n" RESET, index);
          #endif
          return -1;
        }
        #ifdef VERBOSE
        printf(GREEN "Pointer at index %d allocated successfully: %p\n" RESET, index, pointers[index]);
        #endif
        (*allocation_counter)++;
      }
      break;
    }
    case FREE: {
      if (pointers[index] == NULL) {
        #ifdef DEBUG
        printf(RED "Pointer at index %d is already free or not allocated\n" RESET, index);
        #endif
        return -1;
      }
      if (allocator->free(allocator, pointers[index]) == (void *) -1) {
        #ifdef DEBUG
        printf(RED "Failed to free pointer at index %d\n" RESET, index);
        #endif
        return -1;
      }
      (*allocation_counter)--;
      #ifdef VERBOSE
      printf(GREEN "Pointer at index %d freed successfully\n" RESET, index);
      #endif
      pointers[index] = NULL;
      break;
    }
    default:
      #ifdef DEBUG
      printf(RED "Unknown request type: %d\n" RESET, request_type);
      #endif
      return -1;
  }
  return 0;
}
