
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


/*
 * fdNode is a data structure that represents a fd entry.
 */
struct fdNode{
    /*
     * Whether the fdNode is in use
     */
    bool in_use;
    char* filename;

    /*
     * The entry index of the file in the directory
     */
    size_t dir_entry_index;

    size_t size;
    size_t offset;

    /*
     * The first data block of the file. Is FAT_EOC if file has no data blocks.
     */
    size_t first_data_block;
};

struct fdTable{
    struct fdNode* fdTable[FS_OPEN_MAX_COUNT];

    /*
     * Number of slots in fdTable occupied
     */
    size_t fdsOccupied;
};


struct fdNode* getFdEntry(struct fdTable*,int fd);
bool isOpenByFd(struct fdTable*,int fd);

#endif
