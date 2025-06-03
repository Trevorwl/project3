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

void do_sequential_writes();
void do_long_write();
void read_write_exit(char* function,char* message);

/*

 *	do_long_write(): performs a long write on disk
 *
 *	do_sequential_writes(): performs many writes in sequence
 */
int main(int argc, char** argv){
    srand(time(0));
    size_t DATA_BLOCKS = 1 + rand() % BLOCK_LIMIT;

    if(DATA_BLOCKS < 100){
        DATA_BLOCKS = 100;
    }

    unlink("read_write.fs");
    create_disk(DATA_BLOCKS, "read_write.fs");

    if(fs_mount("read_write.fs") == -1){

        read_write_exit("main","error mounting disk");
    }

    do_long_write();
    do_sequential_writes();

    if(fs_umount() == -1){

        read_write_exit("main","error unmounting disk");
   }

    unlink("read_write.fs");

    return EXIT_SUCCESS;
}

/*
 * tests if fs can handle
 * multiple sequential writes
 */
void do_sequential_writes(){
    char* function = "do_sequential_writes";

    if(fs_create("seq_writes") == -1){

        read_write_exit(function,"error creating seq_writes");
    }

    int fd = fs_open("seq_writes");

    if(fd == -1){

        read_write_exit(function,"error opening seq_writes");
    }

    char number_string[32];

    for(int i=0;i<10000;i++){

        memset(number_string, 0, 32 * sizeof(char));

        sprintf(number_string, "%d\n", i);

        if(fs_write(fd,number_string,strlen(number_string)) == -1){

            read_write_exit(function,"error doing sequential writes");
        }
    }

    if(fs_lseek(fd, 0) == -1){

        read_write_exit(function,"lseek error after sequential writes");
    }

    int stat = fs_stat(fd);

    if(stat == -1){

        read_write_exit(function,"stat error after sequential writes");
    }

    char* read_result = (char*)calloc(1,stat + 1);

    read_result[stat]=0;

    if(fs_read(fd, read_result, stat) == -1){

        read_write_exit(function,"read error after sequential writes");
    }

    if(fs_close(fd) == -1){

        read_write_exit(function,"error closing large file");
    }

   char* number_buffer = strtok(read_result, "\n");
   int expected_number = 0;

    while (number_buffer != NULL) {

        if(expected_number > 9999){

            read_write_exit(function,"read more than 10,000 numbers");
        }

        int actual_number = atoi(number_buffer);

        if (actual_number != expected_number) {

            read_write_exit(function,"wrong number read from file");
        }

        expected_number++;
        number_buffer = strtok(NULL, "\n");
    }

    if(expected_number < 10000){
        read_write_exit(function,"read less than 10,000 numbers");
    }

    printf("do_sequential_writes: pass\n");
}

/*
 * tests if fs can do a long write
 */
void do_long_write(){

     char* function = "do_long_write";

     size_t write_buffer_length = 48894;  //bytes required for write

     char* write_buffer = (char*)calloc(1, write_buffer_length);

     size_t write_offset = 0;

     char number_string[32];

     for(int i = 1; i < 10001; i++){

         memset(number_string, 0, 32 * sizeof(char));

         sprintf(number_string, "%d\n", i);

         int number_string_length = strlen(number_string);

         memcpy(&write_buffer[write_offset], number_string, number_string_length);

         write_offset += number_string_length;
     }

      if(fs_create("long_writes")==-1){

          read_write_exit(function,"error creating file long_writes");
      }

      int fd = fs_open("long_writes");

      if(fd==-1){

          read_write_exit(function,"error opening long_writes");
      }

      if(fs_write(fd,write_buffer,write_buffer_length) ==-1){

          read_write_exit(function,"error writing long data to long_writes");
      }

      if(fs_lseek(fd,0) == -1){

          read_write_exit(function,"lseek error after writing long data");
      }

      int stat = fs_stat(fd);

      if(stat == -1){

          read_write_exit(function,"stat error after writing long data");
      }

      char* read_result=(char*)calloc(1,stat + 1);

      read_result[stat]=0;

      if(fs_read(fd,read_result,stat) == -1){

          read_write_exit(function,"error reading long data");
      }

      if(fs_close(fd) ==-1){

          read_write_exit(function,"error closing large file");
      }

      char* number_buffer = strtok(read_result, "\n");
      int expected_number = 1;

     while (number_buffer != NULL) {

         if(expected_number > 10000){

             read_write_exit(function,"more than 10,000 numbers read");
         }

        int actual_number = atoi(number_buffer);

        if (actual_number != expected_number) {

            read_write_exit(function,"wrong number read from file");
        }

        expected_number++;
        number_buffer = strtok(NULL, "\n");
     }
	
     if(expected_number < 10001){

         read_write_exit(function,"less than 10,000 numbers read");
     }

     printf("do_long_write: pass\n");
}

void read_write_exit(char* function,char* message){
    printf("%s: %s\n",function,message);
    unlink("read_write.fs");
    exit(EXIT_FAILURE);
}
