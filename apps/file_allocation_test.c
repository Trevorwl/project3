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

void file_allocation_exit(char* function,char* message);

/*
 * Tests if files are created + allocated and deleted properly
 */
int main(int argc,char** argv){
    srand(time(0));

    size_t REQUESTED_BLOCKS = 100;

    size_t FILES = 23;

    size_t DELETED_FILES = 7;

    size_t file_byte_sizes[] = {17004, 20277, 23363, 9365, 9352,
            3952, 10760, 8830, 14222, 8313, 15659,
            9914, 26965, 30484, 22970, 5630, 19535,
            13239, 18119, 28707, 18722, 1493, 20277};

    size_t deleted_file_indices[] = {2, 4, 8, 12, 15, 17, 20};

    unlink("fa_test.fs");
    create_disk(REQUESTED_BLOCKS,"fa_test.fs");

    if(fs_mount("fa_test.fs")==-1){

        file_allocation_exit("main","mount error");
    }

    char filename[32];

    for(size_t file = 0; file < FILES; file++){

        size_t write_bytes = file_byte_sizes[file];

        uint8_t* file_data_buf = (uint8_t*)calloc(1, write_bytes);

        memset(file_data_buf, '0' + rand() % 10, write_bytes);

        memset(filename, 0, 32 * sizeof(char));

        sprintf(filename,"file%zu.txt",file);

        if(fs_create(filename) == -1){

            file_allocation_exit("main","error creating many files");
        }

        int fd = fs_open(filename);

        if(fd==-1){

            file_allocation_exit("main","error opening one of many files");
        }

        if(fs_write(fd,file_data_buf,write_bytes)==-1){

            file_allocation_exit("main","error writing to one of many files");
        }

        if(fs_close(fd)==-1){

            file_allocation_exit("main","error closing one of many files");
        }

        free(file_data_buf);
    }

    for(size_t file=0; file < DELETED_FILES;file++){

        memset(filename, 0, 32 * sizeof(char));

        sprintf(filename, "file%zu.txt", deleted_file_indices[file]);

        if(fs_delete(filename)==-1){

            file_allocation_exit("main","error deleting one of many files");
        }
    }

    memset(filename, 0, 32 * sizeof(char));
    sprintf(filename,"newfile.txt");

    if(fs_create(filename)==-1){

        file_allocation_exit("main","error adding new file, should have free dir entry");
    }

    int fd=fs_open(filename);

    if(fd==-1){

        file_allocation_exit("main","error opening one of many files");
    }

    char* compare_file = file_as_string("important_tester_file.txt");
    size_t compare_file_bytes = 123680;

    if(fs_write(fd, compare_file, compare_file_bytes)==-1){

        file_allocation_exit("main","error writing large file to seperate blocks");
    }

    char* file_contents=(char*)calloc(1, (compare_file_bytes + 1) * sizeof(char));
    file_contents[compare_file_bytes]=0;

    if(fs_lseek(fd,0)==-1){

        file_allocation_exit("main","lseek error after long write");
    }

    if(fs_read(fd,file_contents,compare_file_bytes)==-1){

        file_allocation_exit("main","error doing long read");
    }

    if(fs_close(fd)==-1){

        file_allocation_exit("main","error closing last file");
    }

    if(fs_umount()==-1){

        file_allocation_exit("main","unmount error");
    }

    int compare = strcmp(file_contents,compare_file);

    free(compare_file);
    free(file_contents);

    unlink("fa_test.fs");

    if(compare == 0){

        printf("file_allocation_test: passed\n");

    } else {
        file_allocation_exit("main","wrote bad data to file");

    }


    unlink("fa_test.fs");

    return EXIT_SUCCESS;
}

void file_allocation_exit(char* function,char* message){
    printf("%s: %s\n",function,message);
    unlink("fa_test.fs");
    exit(EXIT_FAILURE);
}
