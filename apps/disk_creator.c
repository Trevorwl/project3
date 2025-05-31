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

int main (int argc, char** argv){

    if(argc!=3){
        printf("disk_creator: usage: ./disk_creator.x <filename> <blocks>\n");
        return EXIT_FAILURE;
    }

    char* filename=argv[1];

    char *end;
    size_t block_size = strtoul(argv[2], &end, 10);

    if (*end != '\0') {
        printf("disk_creator: error making disk\n");
        return EXIT_FAILURE;
    }

    if(create_disk(block_size,filename)==-1){
        printf("disk_creator: error making disk\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

