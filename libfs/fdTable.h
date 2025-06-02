#ifndef FDTABLE_H_
#define FDTABLE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "fs.h"
#include "disk.h"

#ifndef FS_FILENAME_LEN
#define FS_FILENAME_LEN 16
#endif


#ifndef FS_FILE_MAX_COUNT
#define FS_FILE_MAX_COUNT 128
#endif


#ifndef FS_OPEN_MAX_COUNT
#define FS_OPEN_MAX_COUNT 32
#endif


// fdNode is a data structure that represents a fd entry.
struct fdNode{
    bool in_use; //Whether the fdNode is in use
    char* filename;
    uint16_t dir_entry_index; //The entry index of the file in the directory
    uint32_t size;
    size_t offset;
    uint16_t first_data_block; //The first data block of the file. Is FAT_EOC if file has no data blocks.
};

struct fdTable{
    struct fdNode* fdTable[FS_OPEN_MAX_COUNT];
    size_t fdsOccupied; //Number of slots in fdTable occupied
};

struct fdNode* getFdEntry(struct fdTable*,int fd);
bool isOpenByFd(struct fdTable*,int fd);
bool isOpenByName(struct fdTable *table, const char *filename);
struct fdTable* init_fd_table();

#endif
