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

int main(int argc,char** argv){
    size_t REQUESTED_BLOCKS = 100;

    size_t FILES = 23;

    size_t DELETED_FILES = 7;

    size_t file_byte_sizes[] = {17004, 20277, 23363, 9365, 9352,
            3952, 10760, 8830, 14222, 8313, 15659,
            9914, 26965, 30484, 22970, 5630, 19535,
            13239, 18119, 28707, 18722, 1493, 20277};

    size_t deleted_file_indices[] = {2, 4, 8, 12, 15, 17, 20};

    unlink("fa_test1.fs");
    create_disk(REQUESTED_BLOCKS,"fa_test1.fs");
    fs_mount("fa_test1.fs");

    char name[32];

    for(size_t file = 0; file < FILES; file++){

        size_t write_bytes = file_byte_sizes[file];

        uint8_t* buf = (uint8_t*)calloc(1, write_bytes);
        memset(buf, '0' + rand() % 10, write_bytes);

        memset(name, 0, 32 * sizeof(char));
        sprintf(name,"file%zu.txt",file);

        fs_create(name);
        int fd = fs_open(name);

        fs_write(fd,buf,write_bytes);

        fs_close(fd);

        free(buf);
    }

    for(size_t file=0; file < DELETED_FILES;file++){

        memset(name, 0, 32 * sizeof(char));
        sprintf(name, "file%zu.txt", deleted_file_indices[file]);

        fs_delete(name);
    }

    memset(name, 0, 32 * sizeof(char));
    sprintf(name,"file129.txt");

    fs_create(name);

    int fd=fs_open(name);

    char* compare_file_string = file_as_string("important_tester_file.txt");
    size_t compare_file_bytes = 123680;

    fs_write(fd,compare_file_string,compare_file_bytes);

    char* file_contents=(char*)calloc(1,(123680 + 1)*sizeof(char));
    file_contents[123680]=0;

    fs_lseek(fd,0);

    fs_read(fd,file_contents,123680);

    if(strcmp(file_contents,compare_file_string)==0){
        printf("file_allocation_test: passed\n");
    } else {
        printf("file_allocation_test: failed\n");
    }

    fs_close(fd);

    free(compare_file_string);
    free(file_contents);

    fs_umount();
    unlink("fa_test1.fs");
}
