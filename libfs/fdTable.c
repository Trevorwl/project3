#include "fdTable.h"
#include <stdlib.h>
#include "fs.h"
#include "disk.h"
#include <stdbool.h>
#include <string.h>

struct fdTable* init_fd_table() {
    struct fdTable* table = (struct fdTable*)calloc(1,sizeof(struct fdTable));

    if (!table) return NULL;

    table->fdsOccupied = 0;

    for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
        table->fdTable[i] = NULL;
    }

    return table;
}

struct fdNode* getFdEntry(struct fdTable* fdTable,int fd){
    struct fdNode* fdNode=fdTable->fdTable[fd];

    return fdNode;
}

bool isOpenByFd(struct fdTable* fdTable,int fd){
    struct fdNode* fdNode=fdTable->fdTable[fd];

    if(fdNode==NULL){
        return false;
    }

    return fdNode->in_use==true;
}

bool isOpenByName(struct fdTable *table, const char *filename)
{
    for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
        struct fdNode *node = table->fdTable[i];
        if (node && node->in_use && strcmp(node->filename, filename) == 0) {
            return true;
        }
    }
    return false;
}


