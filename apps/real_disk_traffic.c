#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "disk.h"
#include "fs.h"
#include "utilities.h"
#include <time.h>


char* disk="real_disk_traffic.fs";

void cleanup_and_exit_rdt(char* message){
    printf("%s\n",message);
    unlink(disk);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv){
    long int debug=0;

    if(argc == 2){
        long int input=strtol(argv[1],NULL,10);

        if(input==0 || input==1){
            debug = input;
        }
    }

    srand(time(0));

    unlink(disk);

    size_t disk_blocks = 8001;

    size_t max_file_size =  BLOCK_SIZE * 400;

    size_t random_write_scale = BLOCK_SIZE / 2 + 500;
    size_t random_read_scale = BLOCK_SIZE * 2 - 500;

    create_disk(disk_blocks, disk);

    clock_t start_time = clock();

    int ret = -1;

    int files_in_use[FS_FILE_MAX_COUNT];

    memset(files_in_use, 0, sizeof(int) * FS_FILE_MAX_COUNT);

    for(size_t operation = 0; operation < 10000; operation++){
        printf("doing operation %zu\n",operation);
      

        if(fs_mount(disk)==-1){
            cleanup_and_exit_rdt("fail: didn't mount disk");
        }

        size_t filesFree = free_files();

        if(filesFree == 0 || free_blocks()==0){

            size_t files_to_remove = 1 + rand() % (FS_FILE_MAX_COUNT - filesFree);

            if(debug){printf("freeing %zu files\n",files_to_remove);}

            while(files_to_remove > 0){

                size_t next_removal_file = rand() % FS_FILE_MAX_COUNT;

                while(files_in_use[next_removal_file] == 0){
                   next_removal_file = rand() % FS_FILE_MAX_COUNT;
                }

                char* name = (char*)calloc(1, 128);

                sprintf(name, "file%zu.txt", next_removal_file);

                if(fs_delete(name) == -1){
                    cleanup_and_exit_rdt("fail: error deleting one of many files");
                }

                files_to_remove--;

                files_in_use[next_removal_file]=0;

                free(name);
            }
        }

        if(free_blocks()==0){
            if(debug){printf("disk full\n");}
            do {

                size_t next_removal_file = rand() % FS_FILE_MAX_COUNT;

                while(files_in_use[next_removal_file]==0){
                    next_removal_file = rand() % FS_FILE_MAX_COUNT;
                }

                char* name = (char*)calloc(1,128);

                sprintf(name,"file%zu.txt",next_removal_file);

                if(fs_delete(name) == -1){

                    cleanup_and_exit_rdt("fail: error deleting one of many files");
                }

                files_in_use[next_removal_file]=0;

                free(name);

            } while(free_blocks()==0);

        }

        char* filename = (char*)calloc(1,128);

        size_t fileNum = 0;

        for( ; fileNum < FS_FILE_MAX_COUNT; fileNum++){

            if(files_in_use[fileNum]==0){

                break;
            }
        }

        sprintf(filename,"file%zu.txt",fileNum);
        files_in_use[fileNum]=1;

        size_t file_bytes = rand() % (max_file_size + 1);

        size_t file_blocks = total_block_size(file_bytes);

        size_t expected_bytes_written = 0;

        size_t blocks_remaining = free_blocks();

        if(file_blocks > blocks_remaining){

            expected_bytes_written = blocks_remaining * BLOCK_SIZE;

        }  else {

            expected_bytes_written = file_bytes;
        }

        char* data_buffer = (char*)calloc(1, file_bytes == 0 ? 1 : file_bytes);

        for(size_t c = 0; c < file_bytes; c++){

            data_buffer[c] = 'a' + (char)(rand() % 26);
        }

        ret = fs_create(filename);

        if(ret==-1){
            cleanup_and_exit_rdt("fail: didn't create one of many files");
        }

        int fd = fs_open(filename);

        if(fd==-1){
            cleanup_and_exit_rdt("fail: didn't open one of many files");
        }

        ret = fs_write(fd, data_buffer, file_bytes);

        if(ret != expected_bytes_written){
            cleanup_and_exit_rdt("fail: wrote wrong number of bytes");
        }

        ret = fs_lseek(fd,0);

        if(ret==-1){
            cleanup_and_exit_rdt("fail: lseek error");
        }

        char* cmp_buffer = (char*)calloc(1, file_bytes == 0 ? 1 : file_bytes);

        ret = fs_read(fd,cmp_buffer,file_bytes);

        if(ret != expected_bytes_written){
             cleanup_and_exit_rdt("fail: read wrong number of bytes");
         }

        if(strncmp(data_buffer, cmp_buffer, expected_bytes_written) != 0){

            cleanup_and_exit_rdt("fail: data in file is incorrect");
        }

        if(debug){printf("confirming that %d bytes stored correctly\n",ret);}

        if(fs_close(fd)==-1){

            cleanup_and_exit_rdt("fail: error closing one of many files");
        }

        if((fd=fs_open(filename))==-1){
            cleanup_and_exit_rdt("fail: error opening one of many files");
        }

        int reads = 10;

        char** random_reads = (char**)calloc(1, reads * sizeof(char*));
        size_t* attempted_read_lengths = (size_t*)calloc(1,reads * sizeof(size_t));
        size_t* actual_read_lengths = (size_t*)calloc(1, reads * sizeof(size_t));
        size_t* read_offsets = (size_t*)calloc(1, reads * sizeof(size_t));

        for(size_t r = 0; r < reads;r++){

            attempted_read_lengths[r] =  (expected_bytes_written == 0) ? 0 :
                    (rand() % (expected_bytes_written + 1)) / random_read_scale;

            read_offsets[r] = (expected_bytes_written == 0 ) ? 0 : rand() % expected_bytes_written;

            size_t end_offset = read_offsets[r] + attempted_read_lengths[r];

            if(end_offset > expected_bytes_written){

                actual_read_lengths[r] = expected_bytes_written - read_offsets[r];

            } else {

                actual_read_lengths[r] = attempted_read_lengths[r];

            }

            random_reads[r] = (char*)calloc(1, attempted_read_lengths[r] == 0 ? 1 : attempted_read_lengths[r]);

            memcpy(random_reads[r], &data_buffer[read_offsets[r]], actual_read_lengths[r]);
        }

        for(size_t r = 0; r < reads; r++){

            if(fs_lseek(fd,read_offsets[r])==-1){
                cleanup_and_exit_rdt("fail: lseek error");
            }

            char* cmp_buf=(char*)calloc(1,attempted_read_lengths[r] == 0 ? 1 : attempted_read_lengths[r]);

            int sz = fs_read(fd,cmp_buf, attempted_read_lengths[r]);

            if(sz != actual_read_lengths[r]){
                cleanup_and_exit_rdt("fail: random read returned wrong size");
            }

            if(strncmp(cmp_buf, random_reads[r], actual_read_lengths[r]) != 0){
                cleanup_and_exit_rdt("fail: random read returned bad data");
            }

            if(debug){printf("confirming that %d bytes read randomly correctly\n",sz);}

            free(cmp_buf);
        }

        int writes = reads;

        char** random_writes = (char**)calloc(1, writes * sizeof(char*));
        size_t* attempted_write_lengths = (size_t*)calloc(1,writes * sizeof(size_t));
        size_t* actual_write_lengths = (size_t*)calloc(1,writes * sizeof(size_t));
        size_t* write_offsets = (size_t*)calloc(1,writes * sizeof(size_t));

        size_t data_written = expected_bytes_written;

        size_t allocated_file_blocks = total_block_size(data_written);

        size_t bytes_remaining = (allocated_file_blocks * BLOCK_SIZE - data_written) + free_blocks() * BLOCK_SIZE;

        for(size_t w = 0; w < writes; w++){

            size_t max_possible_bytes = data_written + bytes_remaining;

            write_offsets[w] = data_written == 0 ? 0 : rand() % (data_written + 1);

            attempted_write_lengths[w] =  (rand() % (max_possible_bytes + 1)) / random_write_scale;

            if(attempted_write_lengths[w] < 10){
                attempted_write_lengths[w] = 10;
            }

            size_t end_offset = write_offsets[w] + attempted_write_lengths[w];

            if(end_offset > max_possible_bytes){
                end_offset = max_possible_bytes;
            }

            if(end_offset > data_written){
                bytes_remaining -= (end_offset - data_written);
                data_written = end_offset;
            }

            actual_write_lengths[w] = end_offset - write_offsets[w];

            random_writes[w] = (char*)calloc(1, attempted_write_lengths[w] == 0 ? 1 : attempted_write_lengths[w]);

            for(size_t character = 0; character < attempted_write_lengths[w]; character++){

                random_writes[w][character] = 'a' + (char)(rand() % 26);
            }
        }

        for(size_t w = 0; w < writes; w++){

            if(fs_lseek(fd,write_offsets[w])==-1){
                cleanup_and_exit_rdt("fail: lseek error");
            }

            char* cmp_buf = (char*)calloc(1, attempted_write_lengths[w] == 0 ? 1 : attempted_write_lengths[w]);

            int sz = fs_write(fd,random_writes[w], attempted_write_lengths[w]);

            if(sz != actual_write_lengths[w]){
                cleanup_and_exit_rdt("fail: random write returned wrong size");
            }

            if(fs_lseek(fd,write_offsets[w])==-1){
                 cleanup_and_exit_rdt("fail: lseek error");
             }

            sz = fs_read(fd,cmp_buf,attempted_write_lengths[w]);

            if(sz != actual_write_lengths[w]){
                cleanup_and_exit_rdt("fail: random read returned wrong size");
            }

            if(strncmp(cmp_buf,random_writes[w],actual_write_lengths[w])!=0){
                cleanup_and_exit_rdt("fail: random write not stored correcly");
            }

            if(debug){printf("confirming that %d bytes written randomly correctly\n",sz);}

            free(cmp_buf);
        }

        if(fs_close(fd)==-1){

            cleanup_and_exit_rdt("fail: error closing one of many files");
        }

        free(filename);
        free(data_buffer);
        free(cmp_buffer);

        for(size_t i=0;i<reads;i++){
             free(random_reads[i]);
             free(random_writes[i]);

             random_reads[i]=NULL;
             random_writes[i]=NULL;
         }

        free(random_reads);
        free(random_writes);

        free(attempted_read_lengths);
        free(actual_read_lengths);
        free(read_offsets);

        free(attempted_write_lengths);
        free(actual_write_lengths);
        free(write_offsets);

        if(fs_umount()==-1){
            cleanup_and_exit_rdt("fail: error unmounting disk");
        }
    }



    clock_t end_time = clock();

    double elapsed_seconds = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Elapsed time: %.2f seconds\n", elapsed_seconds);

    printf("pass\n");

    unlink(disk);



    return EXIT_SUCCESS;
}


