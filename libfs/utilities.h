
#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <sys/types.h>

void hex_dump(void *data, size_t length);

int create_disk(size_t data_blocks,char* filename);

void print_allocated_blocks(struct fdNode* fd);

size_t free_blocks();

int load_external_file(char* filename);

char* file_as_string(char* filename);


#endif
