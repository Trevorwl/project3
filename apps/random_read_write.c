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

char* file_as_string(char* filename);

int main(int argc, char** argv){
    size_t EDITS = 1000;

    size_t* write_locations = (size_t*)calloc(1, EDITS * sizeof(size_t));
    size_t* write_lengths = (size_t*)calloc(1, EDITS * sizeof(size_t));

    FILE* write_location_file = fopen("longfile_write_locations.txt", "r");
    FILE* write_lengths_file = fopen("longfile_write_lengths.txt", "r");

    for (int count = 0; count < EDITS; count++ ){

         fscanf(write_location_file, "%zu", &write_locations[count]);
         fscanf(write_lengths_file, "%zu", &write_lengths[count]);
     }

    fclose(write_location_file);
    fclose(write_lengths_file);

    size_t REQUESTED_BLOCKS = 100;

    unlink("arw.fs");
    create_disk(REQUESTED_BLOCKS,"arw.fs");
    fs_mount("arw.fs");

    if(load_external_file("longfile.txt")==-1){
        return EXIT_FAILURE;
    }

    int fs_fd = fs_open("longfile.txt");

    for(size_t edit = 0; edit < EDITS; edit++){

       fs_lseek(fs_fd, write_locations[edit]);

       size_t buf_size = write_lengths[edit];

       char* buf=(char*)calloc(1,buf_size);

       memset(buf,'5',buf_size);

       fs_write(fs_fd,buf,buf_size);

       free(buf);
   }

   bool read_failed=false;

   for(size_t edit = 0; edit < EDITS; edit++){

       fs_lseek(fs_fd, write_locations[edit]);

       size_t buf_size = write_lengths[edit];

       char* buf=(char*)calloc(1, buf_size);

       fs_read(fs_fd,buf,buf_size);

       for(size_t c = 0; c < buf_size; c++){

           if(buf[c] != '5'){

               printf("random_read_write: random reading failed\n");
               read_failed=true;
               break;
           }
       }

       free(buf);

       if(read_failed == true){
           break;
       }
   }

   if(read_failed==false){
       printf("random_read_write: random reading passed\n");
   }

   size_t filesize = fs_stat(fs_fd);

   char* fs_buf=(char*)calloc(1,filesize + 1);

   fs_buf[filesize]=0;

   fs_lseek(fs_fd,0);

   fs_read(fs_fd,fs_buf,filesize);

   char* result_comparison_buf = file_as_string("modified_longfile.txt");

   if(strcmp(result_comparison_buf, fs_buf)!=0){

        printf("random_read_write: random writing failed\n");

   } else {

        printf("random_read_write: random writing passed\n");
   }

   fs_umount();
   unlink("arw.fs");

   free(fs_buf);
   free(result_comparison_buf);
   free(write_locations);
   free(write_lengths);

   return EXIT_SUCCESS;
}

void load_external_file_tester(){
    size_t REQUESTED_BLOCKS=100;
    size_t DATA_BLOCKS=REQUESTED_BLOCKS-1;
    (void)DATA_BLOCKS;

    unlink("arw.fs");
    create_disk(REQUESTED_BLOCKS,"arw.fs");
    fs_mount("arw.fs");

    load_external_file("simple_writer.c");

    int fd = fs_open("simple_writer.c");
    size_t fsize = fs_stat(fd);

    char* buf=(char*)calloc(1,fsize + 1);
    buf[fsize]=0;

    fs_read(fd,buf,fsize);
    fs_close(fd);


    int fd2 = open("simple_writer.c",O_RDONLY);
    struct stat st;
    fstat(fd2, &st);
    off_t filesize = st.st_size;

    char* buf2=(char*)calloc(1, (size_t)filesize + 1);
    buf2[filesize]=0;

    read(fd2,buf2,filesize);

    if(strcmp(buf,buf2)!=0){
        printf("advanced_read_write:fail\n");
    } else {
        printf("advanced_read_write:pass\n");
    }

    fs_umount();

    unlink("arw.fs");

    free(buf);
    free(buf2);
}
