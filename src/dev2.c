#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(){
    int value_size = 16;
    int Nc = 100;

    char *values = (char *) malloc(value_size*Nc*sizeof(char));
    // Filling Values
    for (long i=0; i < Nc; i++){
        // Filling value
        sprintf(values + i*value_size, "hello%ld", i%150);
    }

    printf("First 10 values are\n");
    for (int i=0; i<10; i++) {
        for (int k=0; k < value_size; k++){
            printf("%c", values[i*value_size + k]);
        }
        printf("\n");
    }

    // saving files
    FILE *fvalues = fopen("data/test_values.data", "wb");
    fwrite(values, value_size*sizeof(char), Nc, fvalues);
    printf("Here\n");
    fclose(fvalues);

    // Reading files
    FILE* f;
    char *file_values = (char *) malloc(value_size*Nc*sizeof(char));
    if ((f = fopen("test_values.data", "rb")) == NULL) {
        fprintf(stderr, "can't open file \n");
        exit(1);
    } else {
        printf("Reading values\n");
        fread(file_values, value_size*sizeof(char), Nc, f);
        printf("First 10 values of files are\n");
        for (int i=0; i<10; i++) {
            for (int k=0; k < value_size; k++){
                printf("%c", file_values[i*value_size + k]);
            }
            printf("\n");
        }
        fclose(f);
    }
}