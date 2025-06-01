#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdint.h>

#include "disk.h"
#include "fs.h"

#ifndef FS_OPEN_MAX_COUNT
#define FS_OPEN_MAX_COUNT 32
#endif

/*
 * These 4 variables can be assigned in fs_mount
 */
bool disk_mounted = false;

struct fdTable* fd_table = NULL;

size_t root_directory_index = 0;

size_t data_blocks=0;


int fs_mount(const char *diskname)
{
    (void)diskname;

    return -1;
}

int fs_umount(void)
{
    return -1;
}

int fs_info(void)
{

    return -1;
}

int fs_create(const char *filename)
{
    (void)filename;
	return -1;
}

int fs_delete(const char *filename)
{
    (void)filename;
    return -1;
}

int fs_ls(void)
{

    return -1;
}

int fs_open(const char *filename)
{
    (void)filename;
    return -1;
}

int fs_close(int fd)
{
    (void)fd;
    return -1;
}

int fs_stat(int fd)
{
   (void)fd;
   return -1;
}

int fs_lseek(int fd, size_t offset)
{
    (void)fd;
    (void)offset;
     return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
    if(buf==NULL){
       return -1;
    }

    if(disk_mounted==false){
        return -1;
    }

    if(fd<0 || fd >= FS_OPEN_MAX_COUNT){
        return -1;
    }

    if(isOpenByFd(fd_table,fd)==false){
        return -1;
    }

    if(count<=0){
        return 0;
    }

    struct fdNode* fdEntry = getFdEntry(fd_table,fd);

    if(fdEntry->first_data_block == FAT_EOC){
        //disk full
        if(add_file_to_disk(fdEntry)==false){
            return 0;
        }
    }

    size_t total_block_count = total_file_blocks(fdEntry->first_data_block);

    size_t total_bytes = total_block_count * (size_t)BLOCK_SIZE;

    size_t end_offset = fdEntry->offset + count;

    if(end_offset > total_bytes){

        size_t needed_space = end_offset - total_bytes;

        size_t needed_blocks = total_block_size(needed_space);

        size_t new_blocks = allocate_more_blocks(fdEntry, needed_blocks);

        if(new_blocks==0 && fdEntry->offset==total_bytes){//end of file and no more space allocated
            return 0;
        }

        total_block_count = total_file_blocks(fdEntry->first_data_block);

        total_bytes = total_block_count * (size_t)BLOCK_SIZE;

        if(end_offset > total_bytes){

           end_offset = total_bytes;
        }
    }

    init_bounce_buffer();
    clear_bounce_buffer();

    size_t write_block =  skip_blocks(fdEntry->first_data_block, current_block_offset(fdEntry->offset));

    size_t raw_write_block = get_actual_block_index(write_block);

    size_t bytesWritten = 0;

    size_t character = 0;

    while(fdEntry->offset < end_offset){

         size_t write_characters = 0;

         size_t offset_in_block = fdEntry->offset % (size_t)BLOCK_SIZE;

         if(current_block_offset(fdEntry->offset) < current_block_offset(end_offset)){

             //we will write until end of block
             write_characters = (size_t)BLOCK_SIZE - offset_in_block;

         } else {

             //we will write somewhere within the block
             write_characters = end_offset % (size_t)BLOCK_SIZE - offset_in_block;
         }

         if(write_characters == BLOCK_SIZE){

            block_write(raw_write_block, &buf[character]);

         } else {

            clear_bounce_buffer();

            block_read(raw_write_block, bounce_buffer);

            memcpy(&bounce_buffer[offset_in_block], &buf[character], write_characters);

            block_write(raw_write_block, bounce_buffer);

         }

         fdEntry->offset += write_characters;

         character += write_characters;

         bytesWritten += write_characters;

         if(fdEntry->offset > fdEntry->size){
             fdEntry->size = fdEntry->offset;
         }

         if(fdEntry->offset < end_offset){

             write_block =  skip_blocks(write_block, 1);
             raw_write_block =  get_actual_block_index(write_block);
         }
    }

    clear_bounce_buffer();

    block_read(root_directory_index,bounce_buffer);

    struct DirEntry* dir_entry=(struct DirEntry*)bounce_buffer + fdEntry->dir_entry_index;

    dir_entry->size=(uint32_t)fdEntry->size;

    block_write(root_directory_index,bounce_buffer);

    return bytesWritten;
}

int fs_read(int fd, void *buf, size_t count)
{
    if(buf==NULL){
       return -1;
    }

    if(disk_mounted==false){
        return -1;
    }

    if(fd<0 || fd >= FS_OPEN_MAX_COUNT){
        return -1;
    }

    if(isOpenByFd(fd_table,fd)==false){
        return -1;
    }

    if(count<=0){
        return 0;
    }

    struct fdNode* fdEntry = getFdEntry(fd_table,fd);

    if(fdEntry->first_data_block == FAT_EOC){
        return 0;
    }

    if(fdEntry->offset >= fdEntry->size){
        return 0;
    }

    size_t end_offset = fdEntry->offset + count;

    if(end_offset > fdEntry->size){
        end_offset = fdEntry->size;
    }

    init_bounce_buffer();
    clear_bounce_buffer();

    size_t read_block = skip_blocks(fdEntry->first_data_block, current_block_offset(fdEntry->offset));

    size_t raw_read_block = get_actual_block_index(read_block);

    size_t bytesRead = 0;

    size_t character = 0;

    while(fdEntry->offset < end_offset){

        size_t read_characters = 0;

        size_t offset_in_block = fdEntry->offset % (size_t)BLOCK_SIZE;

        if(current_block_offset(fdEntry->offset) < current_block_offset(end_offset)){

            //we will write until end of block
            read_characters = (size_t)BLOCK_SIZE - offset_in_block;

          } else {

              //we will write somewhere within the block
              read_characters = end_offset % (size_t)BLOCK_SIZE - offset_in_block;
          }

          if(read_characters == BLOCK_SIZE){

             block_read(raw_read_block, &buf[character]);

          } else {

             clear_bounce_buffer();

             block_read(raw_read_block, bounce_buffer);

             memcpy(&buf[character], &bounce_buffer[offset_in_block], read_characters);
          }

          fdEntry->offset += read_characters;

          character += read_characters;

          bytesRead += read_characters;

          if(fdEntry->offset < end_offset){

              read_block =  skip_blocks(read_block, 1);
              raw_read_block =  get_actual_block_index(read_block);
          }
    }

    return bytesRead;
}

