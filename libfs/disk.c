#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

uint16_t FAT_EOC = 0xFFFF;

uint8_t* bounce_buffer = NULL;
size_t bounce_buffer_size = BLOCK_SIZE * sizeof(uint8_t);

uint8_t* disk_buffer = NULL;

#define block_error(fmt, ...) \
	fprintf(stderr, "%s: "fmt"\n", __func__, ##__VA_ARGS__)

/* Invalid file descriptor */
#define INVALID_FD -1

/* Disk instance description */
struct disk {
	/* File descriptor */
	int fd;
	/* Block count */
	size_t bcount;
};

/* Currently open virtual disk (invalid by default) */
static struct disk disk = { .fd = INVALID_FD };

int block_disk_open(const char *diskname)
{
	int fd;
	struct stat st;

	if (!diskname) {
		block_error("invalid file diskname");
		return -1;
	}

	if (disk.fd != INVALID_FD) {
		block_error("disk already open");
		return -1;
	}

	if ((fd = open(diskname, O_RDWR, 0644)) < 0) {
		perror("open");
		return -1;
	}

	if (fstat(fd, &st)) {
		perror("fstat");
		return -1;
	}

	/* The disk image's size should be a multiple of the block size */
	if (st.st_size % BLOCK_SIZE != 0) {
		block_error("size '%llu' is not multiple of '%d'",
		        (unsigned long long)st.st_size, BLOCK_SIZE);
		return -1;
	}

	disk.fd = fd;
	disk.bcount = st.st_size / BLOCK_SIZE;

	return 0;
}

int block_disk_close(void)
{
	if (disk.fd == INVALID_FD) {
		block_error("no disk currently open");
		return -1;
	}

	close(disk.fd);

	disk.fd = INVALID_FD;

	return 0;
}

int block_disk_count(void)
{
	if (disk.fd == INVALID_FD) {
		block_error("no disk currently open");
		return -1;
	}

	return disk.bcount;
}

int block_write(size_t block, const void *buf)
{
	if (disk.fd == INVALID_FD) {
		block_error("no disk currently open");
		return -1;
	}

	if (block >= disk.bcount) {
		block_error("block index out of bounds (%zu/%zu)",
			    block, disk.bcount);
		return -1;
	}

	/* Move to the specified block number */
	if (lseek(disk.fd, block * BLOCK_SIZE, SEEK_SET) < 0) {
		perror("lseek");
		return -1;
	}

	/* Perform the actual write into the disk image */
	if (write(disk.fd, buf, BLOCK_SIZE) < 0) {
		perror("write");
		return -1;
	}

	return 0;
}

int block_read(size_t block, void *buf)
{
	if (disk.fd == INVALID_FD) {
		block_error("no disk currently open");
		return -1;
	}

	if (block >= disk.bcount) {
		block_error("block index out of bounds (%zu/%zu)",
			    block, disk.bcount);
		return -1;
	}

	/* Move to the specified block number */
	if (lseek(disk.fd, block * BLOCK_SIZE, SEEK_SET) < 0) {
		perror("lseek");
		return -1;
	}

	/* Perform the actual read from the disk image */
	if (read(disk.fd, buf, BLOCK_SIZE) < 0) {
		perror("read");
		return -1;
	}

	return 0;
}

bool add_file_to_disk(struct fdNode* fd){
    size_t available_block;

    if(first_block_available(&available_block)==false){
        return false;
    }

    fd->first_data_block = available_block;

    clear_block(fd->first_data_block);

    if(disk_buffer==NULL){
        disk_buffer = (uint8_t*)calloc(1, bounce_buffer_size);
    } else {
        memset(disk_buffer,0,bounce_buffer_size);
    }

    block_read(root_directory_index, disk_buffer);

    struct DirEntry* dirEntry=(struct DirEntry*)disk_buffer + fd->dir_entry_index;

    dirEntry->index = (uint16_t)available_block;

    block_write(root_directory_index, disk_buffer);

    set_fat_entry(dirEntry->index, FAT_EOC);

    return true;
}

int erase_file(size_t data_block_start){
    if((uint16_t)data_block_start == FAT_EOC){
        return 0;
    }

    uint16_t data_block = (uint16_t)data_block_start;

    while(1){

        uint16_t next_block=get_fat_entry(data_block);

        set_fat_entry(data_block,0);

        if(next_block!=FAT_EOC){

            data_block = next_block;

        } else {
            return 0;
        }
    }

    return -1;
}

size_t skip_blocks(size_t data_block_index, size_t number_of_blocks){
    if(number_of_blocks==0 || data_block_index==FAT_EOC){
        return data_block_index;
    }

    uint16_t data_block=(uint16_t)data_block_index;

    for(size_t block_jump = 1; block_jump <= number_of_blocks; block_jump++){

        uint16_t next_block = get_fat_entry(data_block);

        if(next_block != FAT_EOC){

            data_block = next_block;

        } else {
            break;
        }
    }

    return data_block;
}

size_t total_file_blocks(size_t data_block_index){
    if(data_block_index == FAT_EOC){
        return 0;
    }

    size_t block_count = 1;

    uint16_t data_block=(uint16_t)data_block_index;

    while(1){
        uint16_t next_block = get_fat_entry(data_block);

        if(next_block!=FAT_EOC){

            block_count++;

            data_block = next_block;

        } else {
            break;
        }
    }

    return block_count;
}

bool first_block_available(size_t* block_index_holder){

    if(disk_buffer==NULL){
        disk_buffer = (uint8_t*)calloc(1, bounce_buffer_size);
    } else {
        memset(disk_buffer,0,bounce_buffer_size);
    }

    for(size_t fat_block_index = (size_t)FAT_BLOCK_START_INDEX;
            fat_block_index < root_directory_index; fat_block_index++){

        block_read(fat_block_index,disk_buffer);

        uint16_t* fat_entry=(uint16_t*)disk_buffer;

        for(int entry = 0; entry < FAT_ENTRIES; entry++){

            if(*fat_entry==0){

                size_t data_block_index = (fat_block_index - (size_t)FAT_BLOCK_START_INDEX)
                        * (size_t)FAT_ENTRIES + (size_t)entry;

                *block_index_holder  = data_block_index;

                 return true;
            }

            fat_entry++;
        }

        memset(disk_buffer,0,bounce_buffer_size);
    }

    return false;
}

size_t allocate_more_blocks(struct fdNode* fd, size_t needed_blocks){

    size_t file_block_count = total_file_blocks(fd->first_data_block);

    size_t end_block = skip_blocks(fd->first_data_block, file_block_count - 1);

    size_t new_blocks = 0;

    for(size_t block = 1; block <=needed_blocks; block++){

        size_t available_block;

        if(first_block_available(&available_block)==false){
             break;
        }

        clear_block(available_block);

        new_blocks++;

        set_fat_entry(end_block, (uint16_t)available_block);

        set_fat_entry(available_block, FAT_EOC);

        end_block = available_block;
   }

   return new_blocks;
}

size_t total_block_size(size_t size_in_bytes){

    size_t total_blocks = size_in_bytes / (size_t)BLOCK_SIZE;

    if(size_in_bytes % (size_t)BLOCK_SIZE != 0){
        total_blocks++;
    }

    return total_blocks;
}

size_t current_block_offset(size_t current_byte_offset){
    return current_byte_offset / (size_t)BLOCK_SIZE;
}

size_t get_actual_block_index(size_t data_block_index){
    return root_directory_index + 1 + data_block_index;
}

size_t get_fat_block_index(size_t data_block_index){
     return (size_t)FAT_BLOCK_START_INDEX
             + data_block_index / (size_t)FAT_ENTRIES;
}

size_t get_fat_block_entry(size_t data_block_index){
    return data_block_index % (size_t)FAT_ENTRIES;
}

void set_fat_entry(size_t data_block_index, uint16_t value){

    if(disk_buffer==NULL){
        disk_buffer = (uint8_t*)calloc(1, bounce_buffer_size);
    } else {
        memset(disk_buffer,0,bounce_buffer_size);
    }

     size_t fat_block_index = get_fat_block_index(data_block_index);
     size_t fat_block_entry = get_fat_block_entry(data_block_index);

     block_read(fat_block_index, disk_buffer);

     uint16_t* entry_ptr=(uint16_t*)disk_buffer + fat_block_entry;

     *entry_ptr = value;

     block_write(fat_block_index,disk_buffer);
}

uint16_t get_fat_entry(size_t data_block_index){

    if(disk_buffer==NULL){
        disk_buffer = (uint8_t*)calloc(1, bounce_buffer_size);
    } else {
        memset(disk_buffer,0,bounce_buffer_size);
    }

    size_t fat_block_index = get_fat_block_index(data_block_index);
    size_t fat_block_entry = get_fat_block_entry(data_block_index);

    block_read(fat_block_index, disk_buffer);

    uint16_t* entry_ptr=(uint16_t*)disk_buffer + fat_block_entry;

    uint16_t entry_value = *entry_ptr;

    return entry_value;
}

void clear_block(size_t data_block_index){
    if(disk_buffer==NULL){
       disk_buffer = (uint8_t*)calloc(1, bounce_buffer_size);
    } else {
       memset(disk_buffer,0,bounce_buffer_size);
    }

    block_write(get_actual_block_index(data_block_index), disk_buffer);
}

int init_bounce_buffer(){
    if(bounce_buffer==NULL){
        bounce_buffer = (uint8_t*)calloc(1, bounce_buffer_size);

        if(!bounce_buffer){
            return -1;
        }
    }

    return 0;
}

void clear_bounce_buffer(){
    if(bounce_buffer!=NULL){
        memset(bounce_buffer, 0, bounce_buffer_size);
    }
}

void delete_bounce_buffer(){
    if(bounce_buffer!=NULL){
        free(bounce_buffer);
        bounce_buffer=NULL;
    }
}

