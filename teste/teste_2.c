#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../include/t2fs.h"


int main()
{
    printf("\nTESTANDO T2FS...\n\n");

    char path[50];
    char* nome = (char*) malloc(32);
    //identify2(name,50);
    strcpy(nome,"/atiloco/25");

    printf("%s\n",nome);
    struct t2fs_record* i;
    i = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));

    i = getEntry(nome);
    printf("-------------------print_got\n");
    printRecord(i);


}
