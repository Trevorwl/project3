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

size_t BLOCK_LIMIT = 8198;

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
    if(data_blocks==0 || data_blocks > BLOCK_LIMIT){
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

    memset(utilities_buffer,0,bounce_buffer_size);

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

void print_allocated_blocks(struct fdNode* fd){
    size_t current_block=fd->first_data_block;

    printf("Allocated blocks for %s:\n",fd->filename);

    size_t total_blocks=0;

    while(current_block!=FAT_EOC){
        printf("%zu ",current_block);

        current_block = get_fat_entry(current_block);

        total_blocks++;
    }

    printf("\ntotal blocks: %zu\n",total_blocks);
}

size_t free_blocks(){
    if(disk_mounted==false){
        return 0;
    }

    if(utilities_buffer==NULL){
        utilities_buffer = (uint8_t*)calloc(1, bounce_buffer_size);
    } else {
        memset(utilities_buffer,0,bounce_buffer_size);
    }

    size_t free_blocks = 0;

    for(size_t fat_block_index = (size_t)FAT_BLOCK_START_INDEX;
            fat_block_index < root_directory_index; fat_block_index++){

        block_read(fat_block_index, utilities_buffer);

        uint16_t* fat_entry=(uint16_t*)utilities_buffer;

        for(int entry = 0; entry < FAT_ENTRIES; entry++){

            size_t data_block_index = (fat_block_index - (size_t)FAT_BLOCK_START_INDEX)
                           * (size_t)FAT_ENTRIES + (size_t)entry;

            if(data_block_index == data_blocks){
                break;
            }

            if(*fat_entry==0){
                 free_blocks++;
            }

            fat_entry++;
        }

        memset(utilities_buffer,0,bounce_buffer_size);
    }

    return free_blocks;
}

int load_external_file(char* filename){
    if(disk_mounted==false){
        printf("load_external_file: disk not mounted\n");
        return -1;
    }

    int fd = open(filename,O_RDONLY);

    if(fd==-1){
        printf("load_external_file: cant open file\n");
        return -1;
    }

    struct stat st;

    if (fstat(fd, &st) == -1) {
        printf("load_external_file: stat error\n");
        close(fd);
        return -1;
    }

    off_t _filesize = st.st_size;

    if(_filesize<0){
        printf("load_external_file: negative offset\n");
        close(fd);
        return -1;
    }

    if(_filesize==0){
        if(fs_create(filename)==-1){
            printf("load_external_file: file already exists in fs\n");
            close(fd);
            return -1;
         }
         close(fd);
         return 0;
    }

    size_t filesize = _filesize;

    if(total_block_size(filesize) > free_blocks()){
        printf("load_external_file: file doesnt fit on disk\n");
        close(fd);
        return -1;
    }

    if(utilities_buffer==NULL){
        utilities_buffer = (uint8_t*)calloc(1, bounce_buffer_size);
    } else {
        memset(utilities_buffer,0,bounce_buffer_size);
    }

    if(fs_create(filename)==-1){
          printf("load_external_file: file already exists in fs\n");
          close(fd);
          return -1;
    }

    int newFd = fs_open(filename);

    while(1){
        ssize_t bytes_read = read(fd, utilities_buffer, bounce_buffer_size);

        if(bytes_read==0){
            break;
        }

        int ret=fs_write(newFd,utilities_buffer,bytes_read);

        if(ret==-1){
            printf("load_external_file: write failed\n");
            fs_close(newFd);
            close(fd);
            return -1;
        }

    }

    fs_close(newFd);
    close(fd);

    return 0;
}

char* file_as_string(char* filename){
    FILE* file = fopen(filename,"r");

    fseek(file, 0, SEEK_END);

    size_t file_size = (size_t)ftell(file);

    rewind(file);

    char* buffer = (char*)calloc(1,file_size + 1);

    buffer[file_size]=0;

    fread(buffer ,1, file_size, file);

    fclose(file);

    return buffer;
}



