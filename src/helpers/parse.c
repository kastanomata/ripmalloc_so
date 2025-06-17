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
    fprintf(stderr, "Invalid allocator creation command format: %s\n", line);
    return -1;
  }
  if (strcmp(token, "i") != 0) {
    fprintf(stderr, "Invalid allocator creation command prefix: %s\n", token);
    return -1;
  }
  token = strtok(NULL, ",");
  if (!token) {
    fprintf(stderr, "No allocator type specified in command\n");
    return -1;
  }
  // Remove trailing newline from token if present
  token[strcspn(token, "\r\n")] = 0;
  enum AllocatorType type = -1;  // Default to an invalid type
  printf("Parsing allocator type: '%s'\n", token);
  if (strcmp(token, "slab") == 0) {
    type = SLAB_ALLOCATOR;
  } else if (strcmp(token, "buddy") == 0) {
    type = BUDDY_ALLOCATOR;
  } else if (strcmp(token, "bitmap") == 0) {
    type = BITMAP_BUDDY_ALLOCATOR;
  } else {
    fprintf(stderr, "Unknown allocator type: '%s'\n", token);
  }
  return type;
}

union AllocatorConfigData parse_allocator_create_parameters(FILE *file, struct AllocatorConfig *config) {
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    if (line[0] != '%') {
      printf("Allocator creation parameters: %s\n", line);
      break;  
    }
  }
  // the structure of the line is:
  // p,<param1>,<param2>,...
  char *token = strtok(line, ",");
  if (!token || strcmp(token, "p") != 0) {
    fprintf(stderr, "Invalid allocator parameters format: %s\n", line);
  }
  union AllocatorConfigData data = {0};  // Initialize to zero
  if (config->type == SLAB_ALLOCATOR) {
    // Parse slab parameters
    token = strtok(NULL, ",");
    if (!token) {
      fprintf(stderr, "No slab size specified\n");
      return data;  // Return empty data
    }
    data.slab.slab_size = strtoul(token, NULL, 10);
    
    token = strtok(NULL, ",");
    if (!token) {
      fprintf(stderr, "No number of slabs specified\n");
      return data;  // Return empty data
    }
    data.slab.n_slabs = strtoul(token, NULL, 10);
  
  } else if (config->type == BUDDY_ALLOCATOR || config->type == BITMAP_BUDDY_ALLOCATOR) {
    // Parse buddy parameters
    token = strtok(NULL, ",");
    if (!token) {
      fprintf(stderr, "No total size specified for buddy allocator\n");
      return data;  // Return empty data
    }
    data.buddy.total_size = strtoul(token, NULL, 10);
    
    token = strtok(NULL, ",");
    if (!token) {
      fprintf(stderr, "No max levels specified for buddy allocator\n");
      return data;  // Return empty data
    }
    data.buddy.max_levels = strtoul(token, NULL, 10);
    
  } else {
    fprintf(stderr, "Unknown allocator type for parameters: %d\n", config->type);
  }
  
  return data;  // Success
}