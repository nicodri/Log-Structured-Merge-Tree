#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 1000
// TODO: remove hard coding of VALUE_SIZE
#define VALUE_SIZE 10
#define FILENAME_SIZE 16


typedef struct component {
    int *keys;
    char *values;
    int Ne; // number of elements stored
    int S; // capacity
} component;

// First version: finit number of components
typedef struct LSM_tree {
    char *name;
    // C0 and buffer are in main memory
    component *C0;
    component *buffer;
    int Ne; // Total number of key/value tuples stored
    int Nc; // Number of file components, ie components on disk
    int *Cs_Ne; // List of number of elements per disk component
    double *ratios; // TODO: linked list or dynamic array for unlimited case
} LSM_tree;


// Component constructor
void init_component(component * c, int component_size, int value_size){
    c->keys = (int *) malloc(component_size*sizeof(int));
    c->values = (char *) malloc(component_size*value_size*sizeof(char));
    c->Ne = 0;
    c->S = component_size;
}

// Component destructor
void free_component(component *c){
    free(c->keys);
    free(c->values);
    free(c);
}


// Initialize LSM_tree object with metadata and C0
// name need to be at most 7 car int
// TODO: check the validity of the args (ratios, Csize)
void init(LSM_tree *lsm, char name[8], int S0, int buffer_size, int Nc, double* ratios){
    // name
    lsm->name = (char*)malloc(sizeof(char) * 32);
    strcpy(lsm->name, name);
    // Initialize C0 and buffer
    lsm->C0 = (component *) malloc(sizeof(component));
    init_component(lsm->C0, S0, VALUE_SIZE);
    lsm->buffer = (component *) malloc(sizeof(component));
    init_component(lsm->buffer, buffer_size, VALUE_SIZE);

    // Int members
    lsm->Nc = Nc;
    lsm->Ne = 0;
    // TODO: read ratios from a file or input
    lsm->ratios = (double *) malloc(Nc*sizeof(double));
    // TODO: read from disk the number of elements per layer
    lsm->Cs_Ne = (int *) malloc(Nc*sizeof(int));
    for (int i=0; i < Nc; i++){
        lsm->ratios[i] = ratios[i];
        lsm->Cs_Ne[i] = 0;
    }
    // Initialize components on disk:
    // v1: finite number of components
    // Standardized name of files in the local directory name/
    // keys: kC%d.data, component number
    // values: vC%d.data, component number
    
    // lsm->file_components = (char*)malloc(2*Nc*FILENAME_SIZE*sizeof(char));
    // char str[256];
    // FILE *f;
    // for (int i=0; i < Nc; i++){
    //     // TODO: use the name of the lsm to create a subdirectory for the lsm
    //     // Initialize keys file
    //     sprintf(str, "kC%d", i+1);
    //     strcpy(lsm->file_components + 2*i*FILENAME_SIZE, str);
    //     sprintf(str, "kC%d.data", i+1);
    //     f = fopen(str, "wb");
    //     fclose(f);

    //     // // Initialize values file
    //     sprintf(str, "vC%d", i+1);
    //     strcpy(lsm->file_components + (2*i + 1)*FILENAME_SIZE , str);
    //     sprintf(str, "vC%d.data", i+1);
    //     f = fopen(str, "wb");
    //     fclose(f);
    // }
}


void read_disk_component(component* C, char filename_keys[], char filename_values[],
                              int Ne, int S, int value_size){
    // Initializing component
    init_component(C, S, value_size);
    // Reading keys
    FILE *fkeys;
    if ((fkeys = fopen(filename_keys, "rb")) == NULL) {
        fprintf(stderr, "can't open file %s \n", filename_keys);
        exit(1);
    }
    else {
        fread(C->keys, sizeof(int), Ne, fkeys);
        //Debug:
        //for (int i=0; i<20; i++) printf("%ld\n", C[i]);
        fclose(fkeys);
    }

    // Reading values
    FILE *fvalues;
    if ((fvalues = fopen(filename_values, "rb")) == NULL) {
        fprintf(stderr, "can't open file %s \n", filename_values);
        exit(1);
    }
    else {
        fread(C->values, value_size, Ne, fvalues);
        //Debug:
        //for (int i=0; i<20; i++) printf("%ld\n", C[i]);
        fclose(fvalues);
    }
}

void write_disk_component(component *pC, char filename_keys[], char filename_values[],
                          int value_size){
    // Write keys
    FILE *fkeys = fopen(filename_keys, "wb");
    fwrite(pC->keys, sizeof(int), pC->Ne, fkeys);
    fclose(fkeys);

    // Debug: TOFIX
    // printf("First 10 values of C1 to write are\n");
    // for (int i=0; i<10; i++) {
    //     for (int k=0; k < VALUE_SIZE; k++){
    //         printf("%c", pC->values[i*VALUE_SIZE + k]);
    //     }
    //     printf("\n");
    // }

    // Write values: 
    FILE *fvalues = fopen(filename_values, "wb");
    fwrite(pC->values, VALUE_SIZE*sizeof(char), pC->Ne, fvalues);
    fclose(fvalues);
}

void append_lsm(LSM_tree *lsm, int key, char *value){
    // TODO: check validity of the args (size of the value, ...)
    // BUGGY: read wrong here
    // printf("False read inside append: %s\n", lsm->file_components);
    // printf("Printing ratios: ");
    // for (int i=0; i<lsm->Nc; i++) printf("%f ", lsm->ratios[i]);
    // printf("\n");

    // Append to C0
    lsm->C0->keys[lsm->C0->Ne] = key;
    strcpy(lsm->C0->values + lsm->C0->Ne*VALUE_SIZE, value);
    lsm->C0->Ne++;

    // Merging operations
    
    // Check if C0 is full
    if (lsm->C0->Ne >= lsm->C0->S){
        // Sorting C0
        // Inplace sorting of C0->keys and corresponding reorder in C0->values
        merge_sort_with_values(lsm->C0->keys, lsm->C0->values, 0, lsm->C0->S-1, VALUE_SIZE);

        // Merge C0 into buffer
        // case empty buffer
        if (lsm->buffer->Ne == 0){
            // WARNING: pointer do not point to the same size in memory!!!
            // ==> need to reallocate
            // swap pointers (impossible in function because we swap value)
            int *tempk = lsm->buffer->keys;
            lsm->buffer->keys = lsm->C0->keys;
            lsm->C0->keys = tempk;
            char *tempv = lsm->buffer->values;
            lsm->buffer->values = lsm->C0->values;
            lsm->C0->values = tempv;

            // Reallocate memory
            // keys
            // TOFIX: we assume that size of C0 is lower than size of buffer
            // realloc cant raise an error on C0
            realloc(lsm->C0->keys, (lsm->C0->S) * sizeof(int));
            int *tmpk = realloc(lsm->buffer->keys, (lsm->buffer->S) * sizeof(int));
            if (tmpk == NULL)
            {
                // could not realloc, alloc new space and copy
                int *newk = (int *) malloc((lsm->buffer->S) * sizeof(int));
                for (int i=0; i < lsm->buffer->Ne + lsm->C0->Ne; i++) newk[i] = lsm->buffer->keys[i];
                free(lsm->buffer->keys);
                lsm->buffer->keys = newk;
            }
            else
            {
                lsm->buffer->keys = tmpk;
            }

            // values
            // TOFIX: we assume that size of C0 is lower than size of buffer
            // realloc cant raise an error on C0
            realloc(lsm->C0->values, VALUE_SIZE * (lsm->C0->S) * sizeof(char));
            char *tmpv = realloc(lsm->buffer->values, VALUE_SIZE * (lsm->buffer->S) * sizeof(char));
            if (tmpv == NULL)
            {
                // could not realloc, alloc new space and copy
                char *newv = (char *) malloc(VALUE_SIZE * (lsm->buffer->S) * sizeof(char));
                for (int i=0; i < lsm->buffer->Ne + lsm->C0->Ne; i++)strcpy(newv + i*VALUE_SIZE,
                                                                            lsm->buffer->values + i*VALUE_SIZE);
                free(lsm->buffer->values);
                lsm->buffer->values = newv;
            }
            else
            {
                lsm->buffer->values = tmpv;
            }
        }
        else{
            // We don't free the memory in C0, we just update the number of elements
            // in it.
            merge_components(lsm->C0->keys, lsm->buffer->keys, lsm->C0->values,
                             lsm->buffer->values, lsm->C0->Ne, lsm->buffer->Ne,
                             VALUE_SIZE);
        }
        // Updates number of elements
        lsm->buffer->Ne += lsm->C0->Ne;
        lsm->C0->Ne = 0;
    }

    // Check if buffer is full
    // TODO: Build a function to iterate over the component needed to be merged
    // the component on disk is put into a component struct, so exact same behavior
    // as when merging C0 into buffer except we need to write to the disk afterwards.
    // if (lsm->buffer->Ne >= lsm->buffer->S){
    //     // Compute component size
    //     int Csize = lsm->ratios[0] * lsm->buffer->S;
    //     int Cnumber = 0;
    //     // read from disk the content of next component (here C1)
    //     char *filename_keys = lsm->file_components + 2*Cnumber*FILENAME_SIZE;
    //     char *filename_values = lsm->file_components + (2*Cnumber + 1)*FILENAME_SIZE;
    //     char keys_name[FILENAME_SIZE];
    //     sprintf(keys_name, "%s.data", filename_keys);
    //     char values_name[FILENAME_SIZE];
    //     sprintf(values_name, "%s.data", filename_values);
    //     printf("%s\n", values_name);
    //     // Copying next component to memory
    //     component *dC = (component *) malloc(sizeof(component));
    //     read_disk_component(dC, keys_name, values_name,
    //                         lsm->Cs_Ne[Cnumber], Csize, VALUE_SIZE);
    //     // WARNING: hard encoding of the component number
    //     // case empty next component dC
    //     // TODO: manipulate pointer to dC
    //     if (dC->Ne == 0){
    //         // swap pointers (impossible in function because we swap value)
    //         int *tempk2 = lsm->buffer->keys;
    //         lsm->buffer->keys = dC->keys;
    //         dC->keys = tempk2;
    //         char *tempv2 = lsm->buffer->values;
    //         lsm->buffer->values = dC->values;
    //         dC->values = tempv2;
    //     }
    //     else{
    //         // We don't free the memory in buffer (ie prev component), we just update the number of elements
    //         // in it.
    //         int merged_keys[lsm->buffer->Ne + dC->Ne];
    //         int merged_positions[lsm->buffer->Ne + dC->Ne];
    //         merge_components(lsm->buffer->keys, dC->keys, merged_keys, merged_positions,
    //                          lsm->buffer->Ne, dC->Ne);
    //         dC->keys = merged_keys;
    //         // Merging values
    //         dC->values = build_values(lsm->buffer->values, dC->values, merged_positions,
    //                                    lsm->buffer->Ne, dC->Ne, VALUE_SIZE);
    //     }
    //     // Updates number of elements
    //     dC->Ne += lsm->buffer->Ne;
    //     // Update the number in the lsm object
    //     lsm->Cs_Ne[Cnumber] = dC->Ne;
    //     lsm->buffer->Ne = 0;
    //     // Write the component to disk
    //     write_disk_component(dC, keys_name, values_name, VALUE_SIZE);
    // }
}

// Test storing on disk an array
int main(){
    // Creating lsm structure:
    // buffer_size = 3 * C0_size
    char name[] = "test";
    int Nc = 4;
    double ratios[] = {3, 3, 3, 3};

    LSM_tree lsm;
    init(&lsm, name, SIZE, 4*SIZE, Nc, ratios);

    // filling the lsm
    int size_test = 3100;
    char value[VALUE_SIZE];
    // adding even keys
    for (int i=0; i < size_test/2; i++){
        // Filling value
        sprintf(value, "hello%d", (2*i)%150);
        append_lsm(&lsm, (2*i), value);
    }
    // adding odd keys
    for (int i=size_test/2; i < size_test; i++){
        // Filling value
        sprintf(value, "hello%d", (2*(i-size_test/2) + 1)%150);
        append_lsm(&lsm, 2*(i-size_test/2) + 1, value);
    }

    // printf("Read after append: %s\n", lsm.file_components);
    // printf("Read after append: %s\n", lsm.file_components + FILENAME_SIZE);

    printf("Number of elements in C0: %d\n", lsm.C0->Ne);
    printf("Number of elements in buffer: %d\n", lsm.buffer->Ne);
    printf("Number of elements in C1: %d\n", lsm.Cs_Ne[0]);
    // Testing filling of the buffer
    printf("Reading key and value in buffer after merge\n");
    int index = 10;
    printf("index is: %d\n", index);
    printf("key in C0 is %d\n", lsm.C0->keys[index]);
    printf("value in C0 is: ");
    for (int i=0; i < VALUE_SIZE; i++){
        printf("%c", lsm.C0->values[index*VALUE_SIZE + i]);
    }
    printf("\n");
    printf("key in the buffer is %d\n", lsm.buffer->keys[index]);
    printf("value in buffer is: ");
    for (int i=0; i < VALUE_SIZE; i++){
        printf("%c", lsm.buffer->values[index*VALUE_SIZE + i]);
    }
    printf("\n");

    // Print sequence of keys
    printf("First 10 keys of the buffer are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm.buffer->keys[i]);
    }
    printf("Last 10 keys of the buffer are\n");
    for (int i=lsm.buffer->Ne - 10; i < lsm.buffer->Ne; i++){
        printf("%d \n", lsm.buffer->keys[i]);
    }
    printf("First 10 keys of CO are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm.C0->keys[i]);
    }

    // Read keys from C1
    // FILE *fC;
    // int *Ckeys = (int *) malloc((lsm.Cs_Ne[0])*sizeof(int));
    // char filename[] = "kC1.data";
    // if ((fC = fopen(filename, "rb")) == NULL) {
    //     fprintf(stderr, "can't open file %s \n", filename);
    //     exit(1);
    // } else {
    //     printf("Reading keys of C1\n");
    //     printf("Size is %ld\n", lsm.Cs_Ne[0]);
    //     fread(Ckeys, sizeof(int), lsm.Cs_Ne[0], fC);
    //     printf("First 10 keys of C1 are\n");
    //     for (int i=2000; i<2010; i++) printf("%ld\n", Ckeys[i]);
    //     fclose(fC);
    // }

    // Read values from C1
    // char *Cvalues = (char *) malloc(VALUE_SIZE*(lsm.Cs_Ne[0])*sizeof(char));
    // char filename2[] = "kC1.data";
    // if ((fC = fopen(filename2, "rb")) == NULL) {
    //     fprintf(stderr, "can't open file %s \n", filename);
    //     exit(1);
    // } else {
    //     printf("Reading values of C1\n");
    //     fread(Cvalues, VALUE_SIZE*sizeof(char), lsm.Cs_Ne[0], fC);
    //     printf("First 10 values of C1 are\n");
    //     for (int i=0; i<10; i++) {
    //         for (int k=0; k < VALUE_SIZE; k++){
    //             printf("%c", Cvalues[i*VALUE_SIZE + k]);
    //         }
    //         printf("\n");
    //     }
    //     fclose(fC);
    // }
}