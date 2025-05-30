#include "fdTable.h"
#include <stdlib.h>
#include "fs.h"
#include "disk.h"
#include <stdbool.h>
#include <string.h>

struct fdNode* getFdEntry(struct fdTable* fdTable,int fd){
    struct fdNode* fdNode=fdTable->fdTable[fd];

    return fdNode;
}

bool isOpenByFd(struct fdTable* fdTable,int fd){
    struct fdNode* fdNode=fdTable->fdTable[fd];

    return fdNode->in_use==true;
}




