#include "fs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

void do_sequential_writes();
void do_long_write();
void usage();

/*
 * usage:
 *
 * example1: ./exec/tester.x <command> <diskname>
 * 
 * example2: make test args="<command> <diskname>"
 *
 * commands: long_write 
 *           sequential_writes
 *
 *
 *	long write: performs a long write on disk
 *
 *	sequential write: performs many writes in sequence
 */
int main(int argc, char** argv){

    if(argc != 3){
	usage();
	exit(0);
    }

    char* command = argv[1];
    char* diskname= argv[2];

    int ret=fs_mount(diskname);

    if(ret==-1){
	usage();
	exit(0);
    }

    if(strcmp(command,"long_write")==0){
        do_long_write();

    } else if(strcmp(command,"sequential_writes")==0){
        do_sequential_writes();
    } else {
    	usage();
    }

    fs_umount();
}

/*
 * tests if fs can handle
 * multiple sequential writes
 *
 * pass: numbers are printed in ascending order
 */
void do_sequential_writes(){

    fs_create("1_to_9999");

    int fd = fs_open("1_to_9999");

    for(int i=0;i<10000;i++){
        char c[32];

        sprintf(c,"%d\n",i);

        fs_write(fd,c,strlen(c));
    }

    fs_lseek(fd,0);

    size_t stat=fs_stat(fd);
    char* buf=(char*)calloc(1,stat + 1);

    buf[stat]=0;

    fs_read(fd,buf,stat);

    printf("%s",buf);
    printf("\n");

    fs_close(fd);

    fs_delete("1_to_9999");
}

/*
 * tests if fs can do a long write
 *
 * pass: numbers printing in ascending order
 */
void do_long_write(){

     size_t bufLength = 48894;  //bytes required for write

     char* buf = (char*)calloc(1, bufLength);

     size_t offset = 0;

     char storeNum[32];

     for(int i = 1; i < 10001; i++){

         sprintf(storeNum, "%d\n", i);

         int length = strlen(storeNum);

         memcpy(&buf[offset], storeNum, length);

         offset += length;
     }

      fs_create("long_write");

      int fd=fs_open("long_write");

      fs_write(fd,buf,bufLength);

      fs_lseek(fd,0);

      size_t stat=fs_stat(fd);
      char* buf2=(char*)calloc(1,stat + 1);

      buf2[stat]=0;

      fs_read(fd,buf2,stat);

      printf("%s",buf2);
      printf("\n");

      fs_close(fd);

      fs_delete("long_write");
}

void usage(){
    	printf("\n\n------usage: tester.x"
        
                        "\n\n\t./exec/tester.x <command> <diskname>"

                        "\n\n--or--"

                        "\n\n\tmake test args=\"<command> <diskname>\""

                        "\n\n------commands:"
 
                        "\n\n\tlong_write\n\tsequential_writes\n\n\n");

}