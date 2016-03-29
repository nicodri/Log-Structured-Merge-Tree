#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 10000
#define VALUE_SIZE 8

long K[SIZE];
char V[SIZE][VALUE_SIZE];

FILE *fkeys;
FILE *fvalues;

// Test storing on disk an array
// int main(){
//     // filling the keys array
//     char str[8];
//     for (long i=0; i < SIZE; i++){
//         K[i] = i;
//         sprintf(str, "hello%ld", i%100);
//         strcpy(V[i], str);
//     }

//     // Checking value
//     for (int i=0; i<20; i++) {
//         printf("key: %ld, value: %s \n", K[i], V[i]);

//     }

//     fkeys = fopen("keys.data", "wb");
//     fwrite(K, sizeof(long), sizeof(K), fkeys);
//     fclose(fkeys);

//     fvalues = fopen("values.data", "wb");
//     fwrite(K, 8*sizeof(char), sizeof(K), fvalues);
//     fclose(fvalues);

    // reading the array
    // char filename[] = "test.data";
    // if ((fC = fopen(filename, "rb")) == NULL) {
    //     fprintf(stderr, "can't open file %s \n", filename);
    //     exit(1);
    // } else {
    //     fread(C, sizeof(long), sizeof(C), fC);
    //     for (int i=0; i<20; i++) printf("%ld\n", C[i]);
    //     fclose(fC);
    // }

typedef struct LSM_test {
    char *file_components; 
} LSM_test;

LSM_test init(int N, int M){
    LSM_test lsm;
    char str[M];
    for (int i=0; i < N; i++){
        // TODO: use the name of the lsm to create a subdirectory for the lsm
        // Initialize keys file
        sprintf(str, "kC%d", i);
        strcpy(lsm.file_components + i*M, str);
    }
    // Copying the pointer
    // access the keys of component number C_number with
    // BUGGY: read perfectly fine here
    printf("Correct read inside init: %s\n", lsm.file_components);
    printf("Correct read inside init: %s\n", lsm.file_components + M);
    return lsm;
}

int main(){
    LSM_test lsm = init(3, 20);
    printf("Wrong read inside init: %s\n", lsm.file_components);
    printf("Wrong read inside init: %s\n", lsm.file_components + 20);
}