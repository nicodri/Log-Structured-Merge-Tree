#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 1000
#define VALUE_SIZE 10
#define N_COMPONENTS 4


typedef struct component {
    int *keys;
    int *values;
    long Ne; // number of elements stored
    long S; // capacity
} component;

// First version: infinited number of components
typedef struct LSM_tree {
    // C0 and buffer are in main memory
    component C0;
    component buffer;
    long Ne; // Total number of key/value tuples stored
    // long Nc;
    int *ratios; // TODO: linked list or dynamic array for unlimited case
    // List of pointers to the files (at each level: 1 for the keys and 1 for the values)
    FILE **components; // TODO: linked list or dynamic array for unlimited case
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

// Initialize LSM_tree object with metadata and C0
LSM_tree init(long S0, long buffer_size){
    LSM_tree lsm;
    // Initialize C0 and buffer(on memory)
    lsm.C0 = init_component(S0, VALUE_SIZE);
    lsm.buffer = init_component(buffer_size, VALUE_SIZE);

    lsm.Ne = 0;
    lsm.ratios = (int *) malloc(N_COMPONENTS*sizeof(double));
    // TODO: read ratios from a file or input
    lsm.components = malloc(sizeof(FILE*) * 2 * N_COMPONENTS);

    return lsm;
}

void append_lsm(LSM_tree *lsm, long key, char *value){
    // TODO: check validity of the args (size of the value, ...)

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
}

// Test storing on disk an array
int main(){
    // Creating lsm structure:
    // buffer_size = 3 * C0_size
    LSM_tree lsm = init(SIZE, 3*SIZE);

    // filling the lsm
    long size_test = 2100;
    char value[VALUE_SIZE];
    for (long i=0; i < size_test; i++){
        // Filling value
        sprintf(value, "hello%ld", i%150);
        append_lsm(&lsm, i, value);
    }

    printf("Number of elements in C0: %ld\n", lsm.C0.Ne);
    printf("Number of elements in buffer: %ld\n", lsm.buffer.Ne);
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

    // Print elements from the buffer
    printf("First 10 keys of the buffer are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm.buffer.keys[i]);
    }
    printf("First 10 keys of CO are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm.C0.keys[i]);
    }
    printf("key at 200 in C0 is %d\n", lsm.C0.keys[200]);
}