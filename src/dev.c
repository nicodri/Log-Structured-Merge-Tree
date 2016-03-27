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


int main(){
    char a[2][14];
    strcpy(a[0], "blah");
    strcpy(a[1], "hmm");

    printf("%s\n", a[0]);
    if (a[0][3] == '\0') printf("TRUE\n");
    printf("%c\n", a[0][4]);
    printf("%s\n", a[1]);
    char *t = a[0];
    printf("%s\n", t + 14);
    //printf("%s\n", t[1]);
}