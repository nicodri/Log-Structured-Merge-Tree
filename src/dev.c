#include "LSMtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

#define VALUE_SIZE 10


int main(){
    int list[] = {1, 2, 3, 4, 5};

    // write file
    FILE* fout = fopen("dev", "wb");
    fwrite(list, sizeof(int), 5, fout);
    fclose(fout);

    // Updates number of elements in each component in the metadata of the LSM
    fout = fopen("dev", "r+b");
    // Goto the offset of Cs_Ne
    fseek(fout, -2*sizeof(int), SEEK_END);
    fwrite(list, sizeof(int), 1, fout);
    fclose(fout);

    // fclose(fout);

    // Read file
    int* k = (int*) malloc(sizeof(int));
    fout = fopen("dev", "rb");
    for (int i=0; i<5; i++){
        fread(k, sizeof(int), 1, fout);
        printf("%d\n", *k);
    }
    fclose(fout);

}