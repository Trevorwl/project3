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
 * ./read_write.x <diskname>
 * 
 *
 *---tests----
 *
 *	do_long_write(): performs a long write on disk
 *
 *	do_sequential_writes(): performs many writes in sequence
 */
int main(int argc, char** argv){

    if(argc != 2){
        usage();
        exit(0);
    }

    char* diskname= argv[1];

    int ret=fs_mount(diskname);

    if(ret==-1){
        printf("error mounting file\n");
        exit(0);
    }

    do_long_write();
    do_sequential_writes();

    ret=fs_umount();

    if(ret==-1){
        printf("error unmounting file\n");
   }
}

/*
 * tests if fs can handle
 * multiple sequential writes
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

    int stat=fs_stat(fd);

    char* buf=(char*)calloc(1,stat + 1);
    buf[stat]=0;
    fs_read(fd,buf,stat);

    fs_close(fd);
    fs_delete("1_to_9999");

   char* token = strtok(buf, "\n");
   int expected = 0;

    while (token != NULL) {
        int actual = atoi(token);

        if (actual != expected) {

           printf("do_sequential_writes: data written or read improperly\n");

           return;
        }

        expected++;
        token = strtok(NULL, "\n");
    }

    if(expected != 10000){
	printf("do_sequential_writes: data written or read improperly\n");
	return;
    }

    printf("do_sequential_writes: pass\n");
}

/*
 * tests if fs can do a long write
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

      int stat=fs_stat(fd);
      char* buf2=(char*)calloc(1,stat + 1);

      buf2[stat]=0;
      fs_read(fd,buf2,stat);

      fs_close(fd);
      fs_delete("long_write");

      char* token = strtok(buf2, "\n");
      int expected = 1;

     while (token != NULL) {
        int actual = atoi(token);

        if (actual != expected) {

           printf("do_long_write: data written or read improperly\n");

           return;
        }

        expected++;
        token = strtok(NULL, "\n");
     }
	
     if(expected != 10001){

	   printf("do_long_write: data written or read improperly\n");

	   return;
     }

     printf("do_long_write: pass\n");
}

void usage(){
    	printf("\n\n------usage: read_write.x:\n\n\t.\\read_write.x <diskname>\n\n\n");
}
