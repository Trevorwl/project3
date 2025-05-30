#ifndef _DISK_H
#define _DISK_H

#include <stddef.h> /* for size_t definition */
#include <stdbool.h>
#include "fdTable.h"
#include "fs.h"

#define SUPERBLOCK_INDEX 0
#define FAT_BLOCK_START_INDEX 1

/** Size of a disk block in bytes */
#define BLOCK_SIZE 4096

/*Fat entries per fat block*/
#define FAT_ENTRIES 2048

extern uint16_t FAT_EOC;

extern uint8_t* bounce_buffer;
extern size_t bounce_buffer_size;


struct fdNode;

struct __attribute__((__packed__)) DirEntry{
    uint8_t filename[16];
    uint32_t size;
    uint16_t index;
    uint8_t padding[10];
};

/**
 * block_disk_open - Open virtual disk file
 * @diskname: Name of the virtual disk file
 *
 * Open virtual disk file @diskname. A virtual disk file must be opened before
 * blocks can be read from it with block_read() or written to it with
 * block_write().
 *
 * Return: -1 if @diskname is invalid, if the virtual disk file cannot be opened
 * or is already open. 0 otherwise.
 */
int block_disk_open(const char *diskname);

/**
 * block_disk_close - Close virtual disk file
 *
 * Return: -1 if there was no virtual disk file opened. 0 otherwise.
 */
int block_disk_close(void);

/**
 * block_disk_count - Get disk's block count
 *
 * Return: -1 if there was no virtual disk file opened, otherwise the number of
 * blocks that the currently open disk contains.
 */
int block_disk_count(void);

/**
 * block_write - Write a block to disk
 * @block: Index of the block to write to
 * @buf: Data buffer to write in the block
 *
 * Write the content of buffer @buf (%BLOCK_SIZE bytes) in the virtual disk's
 * block @block.
 *
 * Return: -1 if @block is out of bounds or inaccessible or if the writing
 * operation fails. 0 otherwise.
 */
int block_write(size_t block, const void *buf);

/**
 * block_read - Read a block from disk
 * @block: Index of the block to read from
 * @buf: Data buffer to be filled with content of block
 *
 * Read the content of virtual disk's block @block (%BLOCK_SIZE bytes) into
 * buffer @buf.
 *
 * Return: -1 if @block is out of bounds or inaccessible, or if the reading
 * operation fails. 0 otherwise.
 */
int block_read(size_t block, void *buf);

/*
 * Allocates 1 block for a new file.
 *
 * Returns: true if there was disk space, false otherwise
 */
bool add_file_to_disk(struct fdNode* fd);

/*
 * Note: We dont have to use this function since
 * I didnt need it for my part(part 4). I left it just in case
 * it might be helpful.
 *
 *
 * Description:
 *
 * Removes file from fat filesystem.
 *
 * This goes through the chain of fat entries
 * for a file and sets them to zero.
 *
 * Params: first data block of file
 */
int erase_file(size_t first_data_block);

/*
 * Given a data block index, this function will jump traversal_blocks
 * number of blocks through the fat chain, and return the needed block index
 *
 * For example, if data_block_index represents block index 0 of the fat chain,
 * if traversal_blocks=5, disk_seek will return block index 5 of the fat chain.
 */
size_t skip_blocks(size_t data_block_index, size_t traversal_blocks);

/*
 * The total number of blocks in the file
 */
size_t total_file_blocks(size_t data_block_index);

/*
 * This takes a pointer and populates it with the first free data block index
 *
 * Returns: true if a block was found, false if disk is full
 */
bool first_block_available(size_t* block_index_holder);

/*
 * This function will attempt to allocate up to needed_blocks
 * data blocks for a file.
 *
 * Returns: The number of new blocks allocated
 */
size_t allocate_more_blocks(struct fdNode* fd,size_t needed_blocks);

/*
 * The number of data blocks required to hold certain number of bytes
 */
size_t total_block_size(size_t size);

/*
 * If you are on some byte in a file and there are n blocks,
 * this returns the block you are reading/writing ( a number between [0, n) ).
 */
size_t current_block_offset(size_t byte_offset);

/*
 * Returns the overall disk index for a data block
 */
size_t get_actual_block_index(size_t data_block_index);

/*
 * Given a data block, returns the index of the fat
 * block that has its entry
 */
size_t get_fat_block_index(size_t data_block_index);

/*
 * Given a data block, returns where in a
 * fat block its entry will be located
 *
 * Returns: A number 0 to 2047
 */
size_t get_fat_block_entry(size_t data_block_index);

/*
 * Sets a value in the fat for a data block
 */
void set_fat_entry(size_t data_block_index, uint16_t value);

/*
 * Gets a value from the fat for a data block
 */
uint16_t get_fat_entry(size_t data_block_index);

/*
 * Erases a data block
 */
void clear_block(size_t data_block_index);

/*
 * Functions for accessing the bounce buffer
 */
int init_bounce_buffer();
void clear_bounce_buffer();
void delete_bounce_buffer();

#endif /* _DISK_H */

