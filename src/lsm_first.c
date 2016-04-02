#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 1000
// TODO: remove hard coding of VALUE_SIZE
#define VALUE_SIZE 10
#define FILENAME_SIZE 16

// Questions to ask
// 1) Reading part of file on disk (when too large)
// 2) Best way to delete
// 3) Best between finite or unfinite number (practically how do we do them?)
// 4) Priority rule in C: *intpointer*3 ?

typedef struct component {
    int *keys;
    char *values;
    int *Ne; // number of elements stored (point to the int inside the list of the LSMtree)
    int *S; // capacity (point to the int inside the list of the LSMtree)
    char* component_id; //identifier of the component for the filename (Cnumber or buffer)
} component;

// First version: finit number of components
typedef struct LSM_tree {
    char *name;
    // C0 and buffer are in main memory
    component *C0;
    component *buffer;
    int Ne; // Total number of key/value tuples stored
    int Nc; // Number of file components, ie components on disk
    // TODO: linked list for infinite number oc components?
    int *Cs_Ne; // List of number of elements per component: [C0, buffer, C1, C2,...]
    int *Cs_size; // List of number of elements per component: [C0, buffer, C1, C2,...]
} LSM_tree;


// Component constructor
void init_component(component * c, int* component_size, int value_size, int* Ne,
                    char* component_id){
    c->keys = (int *) malloc((*component_size)*sizeof(int));
    c->values = (char *) malloc((*component_size)*value_size*sizeof(char));
    c->Ne = Ne;
    c->S = component_size;
    c->component_id = component_id;
}

// Component destructor
void free_component(component *c){
    free(c->keys);
    free(c->values);
    free(c->component_id);
    free(c);
}

// Allocate and set filename to 'name/component_typecomponent_id.data'
char* get_files_name(char *name, char* component_id, char* component_type){
    char *filename = (char *) malloc(8*sizeof(char) + sizeof(component_id) + sizeof(name));
    sprintf(filename, "%s/%s%s.data", name, component_type, component_id);

    return filename;
}

// Create on disk the files for the disk component
void create_disk_component(char* name, int Nc){
    // Arbitrary size big enough
    char* component_id = (char*) malloc(16*sizeof(char));
    for (int i=1; i<Nc; i++){
        // Building filename
        sprintf(component_id,"C%d", i);
        char *filename_keys = get_files_name(name, component_id, "k");
        char *filename_values = get_files_name(name, component_id, "v");
        fopen(filename_keys, "wb");
        fopen(filename_values, "wb");
    }

}

// allocate memory for lsm struct
// TODO: need to chck the validity of arguments (length of Cs_size ...)
// TODO: Make a generic constructor of lsm object
// (possible once the linked lists have been set up)
void init_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size){
    lsm->name = (char*)malloc(sizeof(name));
    strcpy(lsm->name, name);
    lsm->buffer = (component *) malloc(sizeof(component));
    lsm->C0 = (component *) malloc(sizeof(component));
    // List contains Nc+2 elements: [C0, buffer, C1, C2,...]
    lsm->Cs_size = (int *) malloc((Nc+2)*sizeof(int));
    lsm->Cs_Ne = (int *) malloc((Nc+2)*sizeof(int));
    lsm->Nc = Nc;
    lsm->Ne = 0;
    // TODO: read from disk the number of elements per layer
    for (int i=0; i < (Nc+2); i++){
        lsm->Cs_size[i] = Cs_size[i];
        lsm->Cs_Ne[i] = 0;
    }

}

// Initialize LSM_tree object with metadata and C0
// name need to be at most 7 car int
// TODO: check the validity of the args (ratios, Csize)
void build_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size){
    // allocate memory for lsm tree
    init_lsm(lsm, name, Nc, Cs_size);

    // Initialize C0 and buffer
    init_component(lsm->C0, Cs_size, VALUE_SIZE, lsm->Cs_Ne, "C0");
    init_component(lsm->buffer, Cs_size + 1, VALUE_SIZE, lsm->Cs_Ne + 1,  "buffer");

    // Initialize on disk the disk components
    create_disk_component(name, Nc);
}

// LSM destructor
void free_lsm(LSM_tree *lsm){
    free(lsm->name);
    free(lsm->Cs_Ne);
    free(lsm->Cs_size);
    free_component(lsm->C0);
    free_component(lsm->buffer);
    free(lsm);
}

// TOFIX: need to initialized the files for each component before calling this function
void read_disk_component(component* C, char *name, int* Ne, char *component_id,
                         int* component_size, int value_size){
    // Initialize component
    init_component(C, component_size, value_size, Ne, component_id);
    // Building filename
    char *filename_keys = get_files_name(name, component_id, "k");
    char *filename_values = get_files_name(name, component_id, "v");

    // Reading files
    FILE *fkeys;
    if ((fkeys = fopen(filename_keys, "rb")) == NULL) {
        fprintf(stderr, "can't open file %s \n", filename_keys);
        exit(1);
    }
    else {
        fread(C->keys, sizeof(int), *Ne, fkeys);
        //Debug:
        //for (int i=0; i<20; i++) printf("%ld\n", C[i]);
        fclose(fkeys);
    }
    FILE *fvalues;
    if ((fvalues = fopen(filename_values, "rb")) == NULL) {
        fprintf(stderr, "can't open file %s \n", filename_values);
        exit(1);
    }
    else {
        fread(C->values, value_size, *Ne, fvalues);
        //Debug:
        //for (int i=0; i<20; i++) printf("%ld\n", C[i]);
        fclose(fvalues);
    }
}

void write_disk_component(component *pC, char *name, int value_size){
    // Building filename
    char *filename_keys = get_files_name(name, pC->component_id, "k");
    char *filename_values = get_files_name(name, pC->component_id, "v");

    // Write keys
    FILE *fkeys = fopen(filename_keys, "wb");
    fwrite(pC->keys, sizeof(int), *(pC->Ne), fkeys);
    fclose(fkeys);

    // Write values: 
    FILE *fvalues = fopen(filename_values, "wb");
    fwrite(pC->values, value_size*sizeof(char), *(pC->Ne), fvalues);
    fclose(fvalues);
}

// Function to write lsm tree to disk
// Need to write: CO, buffer and metadata (from struct)
void write_lsm_to_disk(LSM_tree *lsm){
    // Save memory components to disk
    write_disk_component(lsm->C0, lsm->name, VALUE_SIZE);
    write_disk_component(lsm->buffer, lsm->name, VALUE_SIZE);

    // Save metadata of the lsm to disk in file name.data
    char *filename = (char*) malloc(16*sizeof(char) + sizeof(lsm->name));
    sprintf(filename,"%s/meta.data", lsm->name);
    FILE* fout = fopen(filename, "wb");
    fwrite(lsm->name, sizeof(lsm->name), 1, fout);
    fwrite(&lsm->Ne, sizeof(int), 1, fout);
    fwrite(&lsm->Nc, sizeof(int), 1, fout);
    fwrite(lsm->Cs_Ne, sizeof(int), lsm->Nc+2, fout);
    fwrite(lsm->Cs_size, sizeof(int), lsm->Nc+2, fout);

    fclose(fout);
}

// Try to read lsm from disk in its repository: name
void read_lsm_from_disk(LSM_tree *lsm, char *name){
    // Initialize lsm if needed
    if (lsm == NULL) lsm = (LSM_tree*) malloc(sizeof(LSM_tree));
    lsm->name = (char *) malloc(sizeof(name));
    lsm->C0 = (component *) malloc(sizeof(component));
    lsm->buffer = (component *) malloc(sizeof(component));

    // Read meta.data
    char *filename = (char*) malloc(16*sizeof(char) + sizeof(name));
    sprintf(filename,"%s/meta.data", name);
    FILE* fin = fopen(filename, "rb");
    fread(lsm->name, sizeof(name), 1, fin);
    // TODO: assertion on name
    // TODO: int allocation
    fread(&lsm->Ne, sizeof(int), 1, fin);
    fread(&lsm->Nc, sizeof(int), 1, fin);
    lsm->Cs_Ne = (int *) malloc((lsm->Nc + 2)*sizeof(int));
    lsm->Cs_size = (int *) malloc((lsm->Nc + 2)*sizeof(int));
    fread(lsm->Cs_Ne, sizeof(int), lsm->Nc + 2, fin);
    fread(lsm->Cs_size, sizeof(int), lsm->Nc + 2, fin);

    fclose(fin);

    // Read memory components from disk
    init_component(lsm->C0, lsm->Cs_size, VALUE_SIZE, lsm->Cs_Ne, "C0");
    init_component(lsm->buffer, lsm->Cs_size + 1, VALUE_SIZE, lsm->Cs_Ne + 1,
                   "buffer");
    read_disk_component(lsm->C0, lsm->name, lsm->Cs_Ne, "C0", lsm->C0->S, VALUE_SIZE);
    read_disk_component(lsm->buffer, lsm->name, lsm->Cs_Ne + 1, "buffer",
                        lsm->buffer->S, VALUE_SIZE);
}

// TODO: create external function to apply on both keys and values
// Swap the pointer to keys and values between the two components
// Assumes next_component->S > current_component->S
void swap_component_pointer(component *current_component, component *next_component){
    // WARNING: pointer do not point to the same size in memory!!!
    // ==> need to reallocate
    // swap pointers (impossible in function because we swap value)
    int *tempk = next_component->keys;
    next_component->keys = current_component->keys;
    current_component->keys = tempk;
    char *tempv = next_component->values;
    next_component->values = current_component->values;
    current_component->values = tempv;

    // Reallocate memory
    // keys
    // TOFIX: we assume that size of C0 is lower than size of buffer
    // realloc cant raise an error on C0
    realloc(current_component->keys, *(current_component->S) * sizeof(int));
    int *tmpk = realloc(next_component->keys, *(next_component->S) * sizeof(int));
    if (tmpk == NULL)
    {
        // could not realloc, alloc new space and copy
        int *newk = (int *) malloc(*(next_component->S) * sizeof(int));
        for (int i=0; i < *(next_component->Ne) + *(current_component->Ne); i++) newk[i] = next_component->keys[i];
        free(next_component->keys);
        next_component->keys = newk;
    }
    else
    {
        next_component->keys = tmpk;
    }

    // values
    // TOFIX: we assume that size of C0 is lower than size of buffer
    // realloc cant raise an error on C0
    realloc(current_component->values, VALUE_SIZE * *(current_component->S) * sizeof(char));
    char *tmpv = realloc(next_component->values, VALUE_SIZE * *(next_component->S) * sizeof(char));
    if (tmpv == NULL)
    {
        // could not realloc, alloc new space and copy
        char *newv = (char *) malloc(VALUE_SIZE * *(next_component->S) * sizeof(char));
        for (int i=0; i < *(next_component->Ne) + *(current_component->Ne); i++){
            strcpy(newv + i*VALUE_SIZE, next_component->values + i*VALUE_SIZE);
        }
        free(next_component->values);
        next_component->values = newv;
    }
    else
    {
        next_component->values = tmpv;
    }
}

// TODO: same iterative loop starting from C0 (warning for the memory component pointers)
void append_lsm(LSM_tree *lsm, int key, char *value){
    // TODO: check validity of the args (size of the value, ...)

    // Append to C0
    lsm->C0->keys[lsm->Cs_Ne[0]] = key;
    strcpy(lsm->C0->values + lsm->Cs_Ne[0]*VALUE_SIZE, value);
    // TOFIX Writing on disk: just need to append to the C0 file
    // this instruction copies the whole component
    // write_disk_component(lsm->C0, lsm->name, VALUE_SIZE);

    //Increment number of elements
    lsm->Ne++;
    lsm->Cs_Ne[0]++;

    // Merging operations
    
    // Check if C0 is full
    if (lsm->Cs_Ne[0] >= lsm->Cs_size[0]){
        // Step 1
        // Sorting C0
        // Inplace sorting of C0->keys and corresponding reorder in C0->values
        merge_sort_with_values(lsm->C0->keys, lsm->C0->values, 0, lsm->Cs_size[0]-1,
                               VALUE_SIZE);

        if (lsm->buffer->Ne == 0){
            swap_component_pointer(lsm->C0, lsm->buffer);
        }
        else{
            // We don't free the memory in C0, we just update the number of elements
            // in it.
            merge_components(lsm->C0->keys, lsm->buffer->keys, lsm->C0->values,
                             lsm->buffer->values, lsm->Cs_Ne[0], lsm->Cs_Ne[1],
                             VALUE_SIZE);
        }
        // Updates number of elements
        lsm->Cs_Ne[1] += lsm->Cs_Ne[0];
        lsm->Cs_Ne[0] = 0;

        // Updating log on disk
        write_disk_component(lsm->C0, lsm->name, VALUE_SIZE);
        write_disk_component(lsm->buffer, lsm->name, VALUE_SIZE);
    }

    // Step 2: iterative over all the full components
    // First starts with buffer -> C1, then C1 -> C2 ,...
    int current_C_index = 1;
    component* current_component = lsm->buffer;

    // Check if current component is full
    while (lsm->Cs_Ne[current_C_index] >= lsm->Cs_size[current_C_index]){
        // Initialize next component
        char * component_id = (char*) malloc(8*sizeof(char));
        component* next_component = (component *) malloc(sizeof(component));

        // Read next component from disk
        sprintf(component_id, "C%d", current_C_index);
        read_disk_component(next_component, lsm->name, lsm->Cs_Ne + (current_C_index+1),
                            component_id,lsm->Cs_size + (current_C_index+1),
                            VALUE_SIZE);
        printf("Before Merging\n");
        printf("Ne in next Component: %d\n", *(next_component->Ne));
        if (next_component->Ne == 0){
            swap_component_pointer(current_component, next_component);
        }
        else{
            // We don't free the memory in buffer (ie prev component), we just update the number of elements
            // in it.
            merge_components(current_component->keys, next_component->keys,
                             current_component->values, next_component->values,
                             *(current_component->Ne), *(next_component->Ne),
                             VALUE_SIZE);
        }
        printf("Before update\n");
        printf("Ne in current Component: %d\n", *(current_component->Ne));
        printf("Ne in next Component: %d\n", *(next_component->Ne));
        // Updates number of elements
        lsm->Cs_Ne[current_C_index + 1] += lsm->Cs_Ne[current_C_index];
        lsm->Cs_Ne[current_C_index] = 0;

        printf("after update\n");
        printf("Ne in current Component: %d\n", *(current_component->Ne));
        printf("Ne in next Component: %d\n", *(next_component->Ne)); 

        // Write the component to disk
        // TODO: write also done for the buffer (need to do it as a log)
        write_disk_component(next_component, lsm->name, VALUE_SIZE);
        write_disk_component(current_component, lsm->name, VALUE_SIZE);

        // Updates component
        current_C_index += 1;
        // TOFIX: Issue with buffer (because the next line does not change the pointer
        // outside the function)
        // free(current_component->keys);
        // free(current_component->values);
        // Make current component point to next_component
        current_component = next_component;

    }
}

// Binary search of key inside sorted integer array keys[down,..,top]
// return global index in keys if found else -1
int binary_search(int* keys, int key, int down, int top){
    if (top < down) return -1;
    int middle = (top + down)/2;
    if (keys[middle] < key) return binary_search(keys, key, middle+1, top);
    if (keys[middle] > key) return binary_search(keys, key, down, middle-1);
    return middle;
}

// Binary search of the key in component pC
// Set index to 1 and value to the value if index, else set index to 0.
void component_search(component *pC, int key, int* index, char* value){
    *index = binary_search(pC->keys, key, 0, *(pC->Ne)-1);
    if (*index != -1) strcpy(value, pC->values + (*index)*VALUE_SIZE);
}

// Read value of key in LSMTree lsm
// return NULL if key not present
char* read_lsm(LSM_tree *lsm, int key){
    int* index = (int*) malloc(sizeof(int));
    char* value_output = (char*) malloc(VALUE_SIZE*sizeof(char));
    // Linear scan in C0
    printf("Reading C0\n");
    for (int i=0; i < lsm->Cs_Ne[0]; i++){
        if (lsm->C0->keys[i] == key){
            printf("Found in C0\n");
            strcpy(value_output, lsm->C0->values + i*VALUE_SIZE);
            return value_output;
        }
    }

    // Binary search over disk components (buffer and disk components) 
    char * component_id = (char*) malloc(8*sizeof(char));
    component* pC = lsm->buffer;
    for (int i=1; i<lsm->Nc+1; i++){
        if (lsm->Cs_Ne[i] > 0){
            // Case disk component
            if (i != 1){
                sprintf(component_id, "C%d", i-1);
                read_disk_component(pC, lsm->name, lsm->Cs_Ne + i,
                                    component_id, lsm->Cs_size + i,
                                    VALUE_SIZE);
            }
            printf("Reading %s\n", pC->component_id);
            component_search(pC, key, index, value_output);
            if (*index != -1){
                printf("Found in %s\n", pC->component_id);
                return value_output;
            }
        }
        // Initializing the component pointer for disk component
        if (i==1) pC = (component *) malloc(sizeof(component));
    }
    free(value_output);
    return NULL;
}

// Test lsm from disk
// int main(){
//     char name[] = "test";
//     // int Nc = 4;
//     // double ratios[] = {3, 3, 3, 3};

//     // LSM_tree lsm;
//     // build_lsm(&lsm, name, SIZE, 4*SIZE, Nc, ratios);

//     // // Writing lsm to disk
//     // write_lsm_to_disk(&lsm);

//     // Reading from disk
//     LSM_tree* lsm = (LSM_tree*) malloc(sizeof(LSM_tree));
//     read_lsm_from_disk(lsm, name);

//     // Printing
//     printf("LSMTree char\n");
//     printf("Name %s\n", lsm->name);
//     printf("number of components %d\n", lsm->Nc);
//     printf("number of elements %d\n", lsm->Ne);
//     printf("number of elements in C%d: %d\n",0, lsm->Cs_Ne[0]);
//     printf("number of elements in buffer: %d\n",lsm->Cs_Ne[1]);
//     for (int i=2; i< lsm->Nc+2; i++)
//     printf("number of elements in C%d: %d\n",i-1, lsm->Cs_Ne[i]);

//     printf("Last 10 keys of C0 are\n");
//     for (int i=*(lsm->C0->Ne) - 10; i < *(lsm->C0->Ne); i++){
//         printf("%d \n", lsm->C0->keys[i]);
//     }

//     // Printing first element of C2 (read from disk)
//     component * C = (component *) malloc(sizeof(component));
//     read_disk_component(C, name, lsm->Cs_Ne + 3, "C2", lsm->Cs_size + 3, VALUE_SIZE);

//     // Printing
//     printf("First 10 keys of C2 are\n");
//     for (int i=0; i < 10; i++){
//         printf("%d \n", C->keys[i]);
    
//     }
// }

//Test storing on disk an array
int main(){
    // Creating lsm structure:
    // buffer_size = 3 * C0_size
    char name[] = "test";
    int Nc = 4;
    int size_test = 18100;
    // List contains Nc+2 elements: [C0, buffer, C1, C2,...]
    int Cs_size[] = {SIZE, 3*SIZE, 9*SIZE, 27*SIZE, 27*SIZE, 27*SIZE};

    LSM_tree lsm;
    build_lsm(&lsm, name, Nc, Cs_size);

    // filling the lsm
    char value[VALUE_SIZE];

    // Sorted keys
    // for (int i=0; i < size_test; i++){
    //     // Filling value
    //     sprintf(value, "hello%d", i%150);
    //     append_lsm(&lsm, i, value);
    // }    

    // Unsorted keys
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

    printf("Number of elements in LSMTree: %d\n", lsm.Ne);
    printf("Number of elements in C0: %d\n", lsm.Cs_Ne[0]);
    printf("Number of elements in C0: %d\n", *(lsm.C0->Ne));
    printf("Number of elements in buffer: %d\n", lsm.Cs_Ne[1]);
    printf("Number of elements in buffer: %d\n", *(lsm.buffer->Ne));
    printf("Number of elements in C1: %d\n", lsm.Cs_Ne[2]);
    printf("Number of elements in C2: %d\n", lsm.Cs_Ne[3]);
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
    for (int i=*(lsm.buffer->Ne) - 10; i < *(lsm.buffer->Ne); i++){
        printf("%d \n", lsm.buffer->keys[i]);
    }
    printf("First 10 keys of CO are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm.C0->keys[i]);
    }

    // Reading from lsm
    int length = 5;
    int targets[] = {-3, 0, 100, 7000, 60000};
    char* value_read;
    for (int i=0; i<length; i++){
        value_read = read_lsm(&lsm, targets[i]);
        if (value_read != NULL) printf("Reading key: %d; value found: %s\n", targets[i], value_read);
        else printf("Reading key: %d; key not found:\n", targets[i]);
    } 

    // Writing to disk
    write_lsm_to_disk(&lsm);

    // Free memory
    // TOFIX: buggy
    //free_lsm(&lsm);
}


// int main(){
//     // Creating lsm structure:
//     // buffer_size = 3 * C0_size
//     char name[] = "test";
//     int Nc = 4;
//     // List contains Nc+2 elements: [C0, buffer, C1, C2,...]
//     int Cs_size[] = {SIZE, 3*SIZE, 9*SIZE, 27*SIZE, 27*SIZE, 27*SIZE};

//     LSM_tree lsm;
//     build_lsm(&lsm, name, Nc, Cs_size);

//     // Checking number update
//     printf("Number of elements in C0: %d\n", lsm.Cs_Ne[0]);
//     printf("Number of elements in C0: %d\n", *(lsm.C0->Ne));
//     lsm.Cs_Ne[0] = 100;
//     printf("Number of elements in C0: %d\n", lsm.Cs_Ne[0]);
//     printf("Number of elements in C0: %d\n", *(lsm.C0->Ne));

// }

// Testing creating and reading
// int main(){
//     int size = 1000;
//     component * C = (component *) malloc(sizeof(component));
//     C->S = size + 1000;
//     char filename[] = "data";
//     create_disk_component(filename, 2);
//     read_disk_component(C, filename, 1, 0, size, VALUE_SIZE);
//     char value[VALUE_SIZE];
//     for (int i=0; i < size; i++){
//         // Filling value
//         sprintf(value, "hello%d", i%150);
//         strcpy(C->values + i*VALUE_SIZE, value);
//         C->keys[i] = i;
//     }
//     C->Ne = 1000;

//     // Printing
//     printf("Keys\n");
//     for (int i=0; i < 10; i++){
//         printf("%d \n", C->keys[i]);
//     }

//     printf("Values\n");
//     for (int i=0; i < 10; i++){
//         printf("%s \n", C->values + i*VALUE_SIZE);
//     }

//     // Writing to disk
//     write_disk_component(C, filename, 1,VALUE_SIZE);
// }

// //Testing reading component
// int main(){
//     int size = 4000;
//     component * C = (component *) malloc(sizeof(component));
//     char filename[] = "data";
//     read_disk_component(C, filename, size, "C1", size, VALUE_SIZE);

//     // Printing
//     printf("10 first Keys\n");
//     for (int i=0; i < 10; i++){
//         printf("%d \n", C->keys[i]);
//     }

//     printf("10 last Keys\n");
//     for (int i=size-10; i < size; i++){
//         printf("%d \n", C->keys[i]);
//     }

//     printf("10 last Values\n");
//     for (int i=size-10; i < size; i++){
//         printf("%s \n", C->values + i*VALUE_SIZE);
//     }
// }