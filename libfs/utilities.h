
#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <sys/types.h>

void hex_dump(void *data, size_t length);

int create_disk(size_t data_blocks,char* filename);


#endif
