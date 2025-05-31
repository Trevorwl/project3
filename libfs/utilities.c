#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "disk.h"
#include "fs.h"
#include "utilities.h"

uint8_t* utilities_buffer = NULL;

struct __attribute__((__packed__)) metadata{
    uint8_t signature[8];
    uint16_t totalBlocks;
    uint16_t rootDirectoryIndex;
    uint16_t dataStartIndex;
    uint16_t totalDataBlocks;
    uint8_t totalFatBlocks;
};

void hex_dump(void *data, size_t length) {
    unsigned char *byte = (unsigned char *)data;

    for (size_t i = 0; i < length; ++i) {
        if (i % 16 == 0){
            printf("%08zx  ", i);
        }

        printf("%02x ", byte[i]);

        if ((i + 1) % 8 == 0){
            printf(" ");
        }

        if ((i + 1) % 16 == 0 || i + 1 == length) {
            size_t bytes_missing = 16 - ((i + 1) % 16);

            if (bytes_missing < 16) {

                for (size_t j = 0; j < bytes_missing; ++j){
                    printf("   ");
                }
                if (bytes_missing >= 8){
                    printf("  ");
                }
            }

            printf(" |");

            for (size_t j = i - (i % 16); j <= i; j++){

                printf("%c", (byte[j] >= 32 && byte[j] <= 126) ? byte[j] : '.');
            }

            printf("|\n");
        }
    }
}

int create_disk(size_t data_blocks,char* filename){
    if(data_blocks==0 || data_blocks > 8198){
        printf("create_disk: invalid data block total, valid data block total [1,8198]\n");
        return -1;
    }

    int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0644);

    if (fd == -1) {
       printf("create_disk: file already exists\n");
       return -1;
    }

    size_t fat_blocks = data_blocks / (size_t)FAT_ENTRIES;

    if(data_blocks % FAT_ENTRIES !=0){
        fat_blocks += 1;
    }

    if(utilities_buffer==NULL){
        utilities_buffer = (uint8_t*)calloc(1, bounce_buffer_size);
     } else {
        memset(utilities_buffer,0,bounce_buffer_size);
     }

    size_t disk_blocks = 1 + fat_blocks + 1 + data_blocks;

    for(size_t block = 0; block < disk_blocks; block++){
        write(fd, utilities_buffer, bounce_buffer_size);
        memset(utilities_buffer,0,bounce_buffer_size);
    }

    struct metadata* metadata=(struct metadata*)utilities_buffer;

    char* diskFormat="ECS150FS";

    memcpy(metadata->signature, diskFormat, strlen(diskFormat));

    metadata->totalBlocks=(uint16_t)disk_blocks;
    metadata->rootDirectoryIndex= (uint16_t)1 + (uint16_t)fat_blocks;
    metadata->dataStartIndex = (uint16_t)1 + (uint16_t)fat_blocks +  (uint16_t)1;
    metadata->totalDataBlocks=(uint16_t)data_blocks;
    metadata->totalFatBlocks=(uint8_t)fat_blocks;

    lseek(fd, 0 , SEEK_SET);

    write(fd, utilities_buffer, bounce_buffer_size);

    memset(utilities_buffer,0,bounce_buffer_size);

    uint16_t* fat_ptr=(uint16_t*)utilities_buffer;

    *fat_ptr = FAT_EOC;

    lseek(fd, BLOCK_SIZE , SEEK_SET);

    write(fd, utilities_buffer, bounce_buffer_size);

    close(fd);

    printf("created disk with %zu data blocks\n",data_blocks);

    return 0;
}

