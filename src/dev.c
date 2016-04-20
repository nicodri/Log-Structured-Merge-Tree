#include "LSMtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

#define VALUE_SIZE 10


int main(){
    int i = 2;
    FILE* fd = fopen("file.bin","ab");
    fwrite(&i, sizeof(int), 1, fd);
    fclose(fd);
    i = 4;
    fd = fopen("file.bin","ab");
    fwrite(&i, sizeof(int), 1, fd);
    fclose(fd);

    int* j = (int*)malloc(2*sizeof(int));
    FILE* fr = fopen("file.bin","rb");
    fread(j, sizeof(int), 2, fr);
    fclose(fd);
    printf("%d, %d\n", *j, *j +1);

}