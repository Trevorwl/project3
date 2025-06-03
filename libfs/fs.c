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
 * These 3 variables can be assigned in fs_mount
 */

bool disk_mounted = false;
size_t root_directory_index = 0;
struct fdTable* fd_table = NULL;
size_t data_blocks = 0;


// static int fs_mounted = 0; 		//0 unmounted, 1 mounted
struct superblock sb;
uint16_t *fat = NULL;	//pointer to FAT

struct fs_directory_entry root_directory[fs_file_max_count];

// opens virtual disk, reads superblock, verifies 
int fs_mount(const char *diskname)
{
	//check file descriptor was stored successfully
	if(block_disk_open(diskname) < 0) { 
		// fprintf(stderr, "Failed to open disk\n"); //for testing. remove later
		return -1; 
	}

	uint8_t block[BLOCK_SIZE]; // buffer to hold superblock content

	memset(block,0,BLOCK_SIZE);

	// verify block reads 4096 bytes 
	if (block_read(0, block) < 0) {
		// fprintf(stderr, "Failed to read superblock\n"); //for debugging
		block_disk_close();
		return -1;
	}

	// create actual superblock using block[] info
	struct superblock *supBlock = (struct superblock *) block;

	// check supBlock signature matches "ECS150FS"
	if (strncmp(supBlock->signature, fs_signature, fs_signature_length) != 0) {
		//use strncmp since signature has no null terminator
		
		// fprintf(stderr, "Invalid file system signature\n");
		block_disk_close();
		return -1;
	}


	int disk_blocks = block_disk_count();

	/*  Validate superblock data:
		Fat_block_count is nonzero, 
		Root directory block immediately follows the FAT, 
		Data blocks follow the root, 
		All regions fit within the actual disk size.
	*/ 
	if (supBlock->fat_block_count == 0 ||
	    supBlock->root_directory_index != 1 + supBlock->fat_block_count ||
	    supBlock->data_start_index != supBlock->root_directory_index + 1 ||
	    supBlock->data_start_index + supBlock->data_block_amount > disk_blocks) {
	    
	    block_disk_close();
	    return -1;
	}

	// save copy of supBlock's metadata content 
	memcpy(&sb, supBlock, sizeof(struct superblock));

	// printf("FS mounted: %d blocks, FAT = %d blocks\n", sb.total_blocks, sb.fat_block_count);

    fd_table = init_fd_table();  // Make sure this is defined in fdTable.c

    if (fd_table == NULL) {
        block_disk_close();

        memset(&sb, 0, sizeof(struct superblock));

        return -1;
    }

	//load the file allocation table from disk
	fat = calloc(1, sb.fat_block_count * BLOCK_SIZE);

	//verify mem allocation succeeded
	if (!fat) {
		// fprintf(stderr, "Failed to Allocated FAT\n");
		block_disk_close();

		memset(&sb, 0, sizeof(struct superblock));

		free(fd_table);
		fd_table=NULL;

		return -1;
	}

	// copy each FAT block into fat pointer
	for (uint8_t i = 0; i < sb.fat_block_count; i++) {
		
		// verify read succeeded
		if (block_read(i + 1, block) < 0) {
			// fprintf(stderr, "Failed to read FAT block %d\n", i);
		    block_disk_close();

		    memset(&sb, 0, sizeof(struct superblock));

            free(fd_table);
            fd_table=NULL;

			free(fat);
			fat=NULL;

			return -1;
		}
		// if so, copy block (4096 bytes) amount of data into each fat[i]
		memcpy((uint8_t *)fat + (i * BLOCK_SIZE), block, BLOCK_SIZE);

		memset(block, 0, BLOCK_SIZE);
	}

	// read root directory into memory
	if (block_read(sb.root_directory_index, block) < 0) {
		// fprintf(stderr, "Failed to Read root directory block\n");
	    block_disk_close();

	    memset(root_directory, 0, sizeof(struct fs_directory_entry) * fs_file_max_count);

	    memset(&sb, 0, sizeof(struct superblock));

        free(fd_table);
        fd_table=NULL;

        free(fat);
        fat=NULL;

		return -1;
	}

	// copy full block data into root directory 
	memcpy(root_directory, block, BLOCK_SIZE);

    data_blocks = sb.data_block_amount;

    root_directory_index = sb.root_directory_index;

    disk_mounted = true; //supBlock fs was successfully mounted on disk

	return 0;
}

// unmount current mounted fs and close virtual disk
int fs_umount(void)
{
	// check if there's any mounted fs
	if (!disk_mounted) {
		return -1;
	}

	// free FAT's memory & pointer
	free(fat); 
	fat = NULL;

	memset(root_directory, 0, sizeof(struct fs_directory_entry) * fs_file_max_count);
	memset(&sb,0,sizeof(struct superblock));

    free(fd_table);
    fd_table=NULL;

    data_blocks=0;
    root_directory_index=0;
    disk_mounted = false; // reset mount flag

	// close disk file
	if (block_disk_close() < 0) {
		return -1;
	}
	return 0;
}

//displays info about current mounted fs
int fs_info(void)
{
	// check if there's any mounted fs
	if (!disk_mounted) {
		return -1;
	}
	/* Print format based on provided fs_ref.x ref program */
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", sb.total_blocks);
	printf("fat_blk_count=%d\n", sb.fat_block_count);
	printf("rdir_blk=%d\n", sb.root_directory_index);
	printf("data_blk=%d\n", sb.data_start_index);
	printf("data_blk_count=%d\n", sb.data_block_amount);
	
	// count free entries in FAT (value == 0)
	int free_fat = 0;
	for (int i = 0; i < sb.data_block_amount; i++) {
		if (fat[i] == 0) //if entry is empty
			free_fat++;
	}
	printf("fat_free_ratio=%d/%d\n", free_fat, sb.data_block_amount);

	// count free entries in root directory
	int free_root_dir = 0;
	for (int i = 0; i < fs_file_max_count; i++) {
		if (root_directory[i].file_name[0] == '\0') // if entry is being unused
			free_root_dir++;
	}
	printf("rdir_free_ratio=%d/%d\n", free_root_dir, fs_file_max_count);

	return 0;
}


int fs_create(const char *filename)
{
    // validate mounted and filename
    // if(!disk_mounted || filename == NULL || strlen(filename) == 0 || strlen(filename) >= FS_FILENAME_LEN){
    //     return -1;
    // }

    if (!disk_mounted) {
        printf("[DEBUG] fs_create: no disk mounted\n");
        return -1;
    }
    if (!filename || strlen(filename) == 0 || strlen(filename) >= FS_FILENAME_LEN) {
        printf("[DEBUG] fs_create: invalid filename = '%s'\n", filename ? filename : "NULL");
        return -1;
    }

    // Check if filename already exists
    for (int i = 0; i < fs_file_max_count; i++) {
        if (strcmp(root_directory[i].file_name, filename) == 0)
            return -1;  // File already exists
    }

    // find free entry in root dir
    int free_index = -1; 
    for(int i = 0; i < fs_file_max_count; i++){
        if(root_directory[i].file_name[0] == '\0'){
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1){
        return -1;  //no space in root dir
    }

    // init new file entry
    memset(&root_directory[free_index], 0, sizeof(struct fs_directory_entry));

    strcpy(root_directory[free_index].file_name, filename);

//    strncpy(root_directory[free_index].file_name, filename, FS_FILENAME_LEN-1);
//    root_directory[free_index].file_name[FS_FILENAME_LEN - 1] = '\0';
    root_directory[free_index].file_size = 0;
    root_directory[free_index].data_index = FAT_EOC;

    // write the updated root dir to disk
    if (block_write(sb.root_directory_index, root_directory) < 0){
        return -1;
    }

	return 0;
}

int fs_delete(const char *filename)
{
    // validate mounted and filename
    if(!disk_mounted || filename == NULL || strlen(filename) == 0 || strlen(filename) >= FS_FILENAME_LEN){
        return -1;
    }

    int file_index = -1;
    // find file in root dir
    for(int i = 0; i < fs_file_max_count; i++){
        if (strcmp(root_directory[i].file_name, filename) == 0) {
            file_index = i;
            break;
        }
    }

    if (file_index == -1){
        return -1;  // file not found
    }

    // check if file is open
    if (isOpenByName(fd_table, filename)){
        return -1;
    }
    
    // free all FAT blocks linked to file
    uint16_t current = root_directory[file_index].data_index;
    while(current != FAT_EOC){
        uint16_t next = fat[current];
        fat[current] = 0;
        current = next;
    }


    for(size_t fblock = 0; fblock < sb.fat_block_count;fblock++){
        uint8_t* fat_block=(uint8_t*)fat + fblock * BLOCK_SIZE;
        block_write(FAT_BLOCK_START_INDEX + fblock,fat_block);
    }

    // clear root dir entry
    memset(&root_directory[file_index], 0, sizeof(struct fs_directory_entry));

    // write root dir back to disk
    if (block_write(sb.root_directory_index, root_directory) < 0){
        return -1;
    }

    return 0;
}

int fs_ls(void)
{

    // validate filesystem is mounted
    if (!disk_mounted) {
        return -1;
    }

    // go through all root dir entries
    for (int i = 0; i < fs_file_max_count; i++) {
        if (root_directory[i].file_name[0] != '\0') {
            printf("file: %s, size: %d, data_blk: %d\n",
                   root_directory[i].file_name,
                   root_directory[i].file_size,
                   root_directory[i].data_index);
        }
    }

    return 0;
}

int fs_open(const char *filename)
{
    // validate mounted and filename
    if (!disk_mounted || 
        filename == NULL || 
        strlen(filename) == 0 || 
        strlen(filename) >= FS_FILENAME_LEN) {
        return -1;
    }

    int dir_index = -1;

    // find file in root dir
    for (int i = 0; i < fs_file_max_count; i++) {
        if (strcmp(root_directory[i].file_name, filename) == 0) {
            dir_index = i;
            break;
        }
    }

    if (dir_index == -1) {
        return -1;  // file not found
    }

    // find unused fd slot
    int fd = -1;
    for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
        if (fd_table->fdTable[i] == NULL) {
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        return -1; // no available fd slot
    }

    // allocate and fill fdNodes
    struct fdNode *new_fd = calloc(1,sizeof(struct fdNode));
    if (!new_fd) {
        return -1;
    }

    new_fd->filename = strdup(filename);
    if (!new_fd->filename) {
        free(new_fd);
        return -1;
    }

    new_fd->in_use = true;
    new_fd->dir_entry_index = dir_index;
    new_fd->offset = 0;
    new_fd->size = root_directory[dir_index].file_size;
    new_fd->first_data_block = root_directory[dir_index].data_index;

    // save in fdTable
    fd_table->fdTable[fd] = new_fd;
    fd_table->fdsOccupied++;

    return fd;
}

int fs_close(int fd)
{

    // validate mount & fd valid
    if (!disk_mounted || 
        !fd_table || 
        fd < 0 || 
        fd >= FS_OPEN_MAX_COUNT || 
        !fd_table->fdTable[fd] || 
        !fd_table->fdTable[fd]->in_use) {

        printf("[DEBUG] Invalid fd or fd table\n");
        return -1;
    }

    struct fdNode *node = fd_table->fdTable[fd];

    // check if fd is actually open
    if (node == NULL || node->in_use == false) {
        return -1;
    }

    // free resources
    free(node->filename);
    free(node);

    // mark slot unused
    fd_table->fdTable[fd] = NULL;
    fd_table->fdsOccupied--;

    return 0;
}

int fs_stat(int fd)
{
    // validate mount & fd valid
    if (!disk_mounted || 
        !fd_table || 
        fd < 0 || 
        fd >= FS_OPEN_MAX_COUNT || 
        !fd_table->fdTable[fd] || 
        !fd_table->fdTable[fd]->in_use) {

        printf("[DEBUG] Invalid fd or fd table\n");
        return -1;
    }

    // check if fd is open
    struct fdNode *node = fd_table->fdTable[fd];
    if (node == NULL || !node->in_use) {
        return -1;
    }

    // return the current file size
    return (int) node->size;
}

int fs_lseek(int fd, size_t offset)
{

    // validate mount & fd valid
    if (!disk_mounted || 
        !fd_table || 
        fd < 0 || 
        fd >= FS_OPEN_MAX_COUNT || 
        !fd_table->fdTable[fd] || 
        !fd_table->fdTable[fd]->in_use) {

        printf("[DEBUG] Invalid fd or fd table\n");
        return -1;
    }

    // get fd entry
    struct fdNode *node = fd_table->fdTable[fd];
    if (node == NULL || !node->in_use) {
        printf("[DEBUG] FD %d is not in use\n", fd);
        return -1;
    }

    // check offset is within file size
    if (offset < 0 || offset > node->size) {
        printf("[DEBUG] Offset %zu is beyond file size %zu\n", offset, (size_t)node->size);
        return -1;
    }

    // set new offset
    node->offset = offset;

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

            block_write(raw_write_block, &(((char*)buf)[character]));
//            memcpy(&bounce_buffer[offset_in_block], &(((char*)buf)[character]), write_characters);

         } else {

            clear_bounce_buffer();

            block_read(raw_write_block, bounce_buffer);

            memcpy(&bounce_buffer[offset_in_block], &(((char*)buf)[character]), write_characters);

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

    root_directory[fdEntry->dir_entry_index].file_size = (uint32_t)fdEntry->size;

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

            block_read(raw_read_block, &(((char*)buf)[character]));
//            memcpy(&(((char*)buf)[character]), &bounce_buffer[offset_in_block], read_characters);

          } else {

             clear_bounce_buffer();

             block_read(raw_read_block, bounce_buffer);

             memcpy(&(((char*)buf)[character]), &bounce_buffer[offset_in_block], read_characters);
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

