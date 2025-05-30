#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "utilities.h"

void hex_dump(void *data, size_t length) {
    unsigned char *byte = (unsigned char *)data;

    for (size_t i = 0; i < length; ++i) {
        if (i % 16 == 0){
            printf("%08zx  ", i);
        }

        printf("%02x ", byte[i]);

        if ((i + 1) % 8 == 0){
            printf(" ");
        }

        if ((i + 1) % 16 == 0 || i + 1 == length) {
            size_t bytes_missing = 16 - ((i + 1) % 16);

            if (bytes_missing < 16) {

                for (size_t j = 0; j < bytes_missing; ++j){
                    printf("   ");
                }
                if (bytes_missing >= 8){
                    printf("  ");
                }
            }

            printf(" |");

            for (size_t j = i - (i % 16); j <= i; j++){

                printf("%c", (byte[j] >= 32 && byte[j] <= 126) ? byte[j] : '.');
            }

            printf("|\n");
        }
    }
}

