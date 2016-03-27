#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 1000
// TODO: remove hard coding of VALUE_SIZE
#define VALUE_SIZE 10
#define FILENAME_SIZE 20


typedef struct component {
    int *keys;
    int *values;
    long Ne; // number of elements stored
    long S; // capacity
} component;

// First version: finit number of components
typedef struct LSM_tree {
    char *name;
    // C0 and buffer are in main memory
    component C0;
    component buffer;
    long Ne; // Total number of key/value tuples stored
    long Nc; // Number of file components, ie components on disk
    long *Cs_Ne; // List of number of elements per disk component
    double *ratios; // TODO: linked list or dynamic array for unlimited case
    // List of pointers to the files (at each level: 1 for the keys and 1 for the values)
    // TODO: stores directly a pointer to the files (currently stores the filename)
    char *file_components; 
} LSM_tree;


// Initialize component
component init_component(long component_size, long value_size){
    component c;
    c.keys = (int *) malloc(component_size*sizeof(long));
    c.values = (int *) malloc(component_size*value_size*sizeof(char));
    c.Ne = 0;
    c.S = component_size;

    return c;
}

component read_disk_component(char filename_keys[], char filename_values[],
                              long Ne, long S, long value_size){
    component C = init_component(Ne, S);
    // Reading keys
    FILE *fkeys;
    if ((fkeys = fopen(filename_keys, "rb")) == NULL) {
        fprintf(stderr, "can't open file %s \n", filename_keys);
        exit(1);
    }
    else {
        fread(C.keys, sizeof(long), Ne, fkeys);
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
        fread(C.values, value_size, Ne, fvalues);
        //Debug:
        //for (int i=0; i<20; i++) printf("%ld\n", C[i]);
        fclose(fvalues);
    }
    return C;

}

void write_disk_component(component *pC, char filename_keys[], char filename_values[], long value_size){
    // Write keys
    FILE *fkeys = fopen(filename_keys, "wb");
    fwrite(pC->keys, sizeof(long), pC->Ne, fkeys);
    fclose(fkeys);

    // Write values
    FILE *fvalues = fopen(filename_values, "wb");
    fwrite(pC->values, sizeof(value_size), pC->Ne, fvalues);
    fclose(fvalues);
}

// Initialize LSM_tree object with metadata and C0
// name need to be at most 7 car long
// TODO: check the validity of the args (ratios, Csize)
LSM_tree init(char name[8], long S0, long buffer_size, long Nc, double* ratios){
    LSM_tree lsm;
    lsm.name = name;
    // Initialize C0 and buffer(on memory)
    lsm.C0 = init_component(S0, VALUE_SIZE);
    lsm.buffer = init_component(buffer_size, VALUE_SIZE);

    lsm.Nc = Nc;
    lsm.Ne = 0;
    // TODO: read ratios from a file or input
    lsm.ratios = (double *) malloc(Nc*sizeof(double));
    lsm.Cs_Ne = (long *) malloc(Nc*sizeof(long));
    for (int i=0; i < Nc; i++){
        lsm.ratios[i] = ratios[i];
        lsm.Cs_Ne[i] = 0;
    }
    // Initialize components on disk:
    // v1: finite number of components
    // TODO: handle array of chars
    // BUGGY: the string in file_components are not properly read by next functions
    lsm.file_components = (char *) malloc(2*Nc*FILENAME_SIZE*sizeof(char));
    char filename[2*Nc][FILENAME_SIZE];
    FILE *f;
    int k;
    for (int i=0; i < Nc; i++){
        // TODO: use the name of the lsm to create a subdirectory for the lsm
        // Initialize keys file
        sprintf(filename[2*i], "kC%d.data", i+1);
        f = fopen(filename[2*i], "wb");
        fclose(f);
        // copy filename into file_components
        //  TODO: improve this way of doing
        k = 0;
        while (filename[2*i][k] != '\0'){
            lsm.file_components[2*i + k] = filename[2*i][k];
            k++;
        }

        // Initialize values file
        sprintf(filename[2*i + 1], "vC%d.data", i+1);
        f = fopen(filename[2*i + 1], "wb");
        fclose(f);
        // copy filename into file_components
        //  TODO: improve this way of doing
        k = 0;
        while (filename[2*i][k] != '\0'){
            lsm.file_components[2*i + k] = filename[2*i][k];
            k++;
        }
    }
    // Copying the pointer
    // access the keys of component number C_number with
    lsm.file_components = filename[0];
    // BUGGY: read perfectly fine here
    printf("Correct read inside init: %s\n", lsm.file_components);
    return lsm;
}

void append_lsm(LSM_tree *lsm, long key, char *value){
    // TODO: check validity of the args (size of the value, ...)
    // BUGGY: read wrong here
    // printf("False read inside append: %s\n", lsm->file_components);
    // printf("Printing ratios: ");
    // for (int i=0; i<lsm->Nc; i++) printf("%f ", lsm->ratios[i]);
    // printf("\n");

    // Append to C0
    component *C0 = &lsm->C0;
    C0->keys[C0->Ne] = key;
    // value is array of char => need to copy each char
    for (int i = 0; i < VALUE_SIZE; i++){
        C0->values[VALUE_SIZE*(C0->Ne) + i] = value[i];
    }
    C0->Ne++;

    // Merging operations
    
    // Check if C0 is full
    if (C0->Ne >= C0->S){
        // Sorting C0
        // Initializing positions
        int positions[C0->S];
        // TODO: removing this initialization as it's always the same
        for (int i=0; i<C0->S; i++) positions[i] = i;
        // Inplace sorting of C0->keys and corresponding argsort in positions
        merge_sort_with_positions(C0->keys, positions, 0, C0->S-1);
        // updating values
        // TODO: updates values AFTER the merge to avoid moving twice values
        update_values(C0->values, positions, VALUE_SIZE, C0->S);

        // Merge C0 into buffer
        component *buffer = &lsm->buffer;
        // case empty buffer
        if (buffer->Ne == 0){
            // swap pointers (impossible in function because we swap value)
            int *temp = buffer->keys;
            buffer->keys = C0->keys;
            C0->keys = temp;
            temp = buffer->values;
            buffer->values = C0->values;
            C0->values = temp;
        }
        else{
            // We don't free the memory in C0, we just update the number of elements
            // in it.
            int merged_keys[C0->Ne + buffer->Ne];
            int merged_positions[C0->Ne + buffer->Ne];
            merge_components(C0->keys, buffer->keys, merged_keys, merged_positions,
                           C0->Ne, buffer->Ne);
            buffer->keys = merged_keys;
            // Merging values
            buffer->values = build_values(C0->values, buffer->values, merged_positions,
                                          C0->Ne, buffer->Ne, VALUE_SIZE);
        }
        // Updates number of elements
        buffer->Ne += C0->Ne;
        C0->Ne = 0;
    }

    // Check if buffer is full
    component *buffer = &lsm->buffer;
    // TODO: Build a function to iterate over the component needed to be merged
    // the component on disk is put into a component struct, so exact same behavior
    // as when merging C0 into buffer except we need to write to the disk afterwards.
    if (buffer->Ne >= buffer->S){
        // Compute component size
        long Csize = lsm->ratios[0] * buffer->S;
        int Cnumber = 0;
        // read from disk the content of next component (here C1)
        // BUGGY
        //char * filename_keys = (lsm->file_components) + 2*Cnumber*FILENAME_SIZE;
        //char * filename_values = lsm->file_components + (2*Cnumber + 1)*FILENAME_SIZE;
        char filename_keys[] = "kC1.data";
        char filename_values[] = "kC1.data";
        component dC = read_disk_component(filename_keys, filename_values, lsm->Cs_Ne[Cnumber], Csize, VALUE_SIZE);
        // WARNING: hard encoding of the component number
        
        // case empty next component dC
        // TODO: manipulate pointer to dC
        component *pDc = &dC;
        if (pDc->Ne == 0){
            // swap pointers (impossible in function because we swap value)
            int *temp = buffer->keys;
            buffer->keys = pDc->keys;
            pDc->keys = temp;
            temp = buffer->values;
            buffer->values = pDc->values;
            pDc->values = temp;
        }
        else{
            // We don't free the memory in buffer (ie prev component), we just update the number of elements
            // in it.
            int merged_keys[buffer->Ne + pDc->Ne];
            int merged_positions[buffer->Ne + pDc->Ne];
            merge_components(buffer->keys, pDc->keys, merged_keys, merged_positions,
                             buffer->Ne, pDc->Ne);
            pDc->keys = merged_keys;
            // Merging values
            pDc->values = build_values(buffer->values, pDc->values, merged_positions,
                                       buffer->Ne, pDc->Ne, VALUE_SIZE);
        }
        // Updates number of elements
        pDc->Ne += buffer->Ne;
        buffer->Ne = 0;

        // Write the component to disk
        write_disk_component(pDc, filename_keys, filename_values, VALUE_SIZE);
    }
}

// Test storing on disk an array
int main(){
    // Creating lsm structure:
    // buffer_size = 3 * C0_size
    char name[] = "test";
    int Nc = 4;
    double ratios[] = {3, 3, 3, 3};

    LSM_tree lsm = init(name, SIZE, 3*SIZE, Nc, ratios);

    // filling the lsm
    long size_test = 3100;
    char value[VALUE_SIZE];
    for (long i=0; i < size_test; i++){
        // Filling value
        sprintf(value, "hello%ld", i%150);
        append_lsm(&lsm, i, value);
    }

    printf("Number of elements in C0: %ld\n", lsm.C0.Ne);
    printf("Number of elements in buffer: %ld\n", lsm.buffer.Ne);
    printf("Number of elements in C1: %ld\n", lsm.Cs_Ne[0]);
    // Testing filling of the buffer
    printf("Reading key and value in buffer after merge\n");
    long index = 10;
    printf("index is: %ld\n", index);
    printf("key in C0 is %d\n", lsm.C0.keys[index]);
    printf("value in C0 is: ");
    for (int i=0; i < VALUE_SIZE; i++){
        printf("%c", lsm.C0.values[index*VALUE_SIZE + i]);
    }
    printf("\n");
    printf("key in the buffer is %d\n", lsm.buffer.keys[index]);
    printf("value in buffer is: ");
    for (int i=0; i < VALUE_SIZE; i++){
        printf("%c", lsm.buffer.values[index*VALUE_SIZE + i]);
    }
    printf("\n");

    // Print sequence of keys
    printf("First 10 keys of the buffer are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm.buffer.keys[i]);
    }
    printf("First 10 keys of CO are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm.C0.keys[i]);
    }

    // Read from C1
    // FILE *fC;
    // long Ckeys[lsm.Cs_Ne[0]];
    // char filename[] = "kC1.data";
    // if ((fC = fopen(filename, "rb")) == NULL) {
    //     fprintf(stderr, "can't open file %s \n", filename);
    //     exit(1);
    // } else {
    //     fread(Ckeys, sizeof(long), sizeof(Ckeys), fC);
    //     printf("First 10 keys of C1 are\n");
    //     for (int i=0; i<10; i++) printf("%ld\n", Ckeys[i]);
    //     fclose(fC);
    // }
}