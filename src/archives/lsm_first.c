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
    // Valgrind modif: malloc to calloc (because of padding)
    c->values = (char *) calloc((*component_size)*value_size, sizeof(char));
    c->Ne = Ne;
    c->S = component_size;
    c->component_id = (char *) malloc(sizeof(component_id));
    strcpy(c->component_id, component_id);
}

// Component destructor: no need to free the value pointed by S and Ne
// as they are beeing freed by the lsm
void free_component(component *c){
    free(c->keys);
    free(c->values);
    free(c->component_id);
    free(c);
}

// Allocate and set filename to 'name/component_typecomponent_id.data'
char* get_files_name(char *name, char* component_id, char* component_type){
    char *filename = (char *) calloc(FILENAME_SIZE + 8,sizeof(char));
    sprintf(filename, "%s/%s%s.data", name, component_type, component_id);

    return filename;
}

// Create on disk the files for the disk component
void create_disk_component(char* name, int Nc){
    char *filename_keys, *filename_values;
    // Arbitrary size big enough
    char* component_id = (char*) malloc(16*sizeof(char));
    for (int i=1; i<Nc; i++){
        // Building filename
        sprintf(component_id,"C%d", i);
        filename_keys = get_files_name(name, component_id, "k");
        filename_values = get_files_name(name, component_id, "v");
        fopen(filename_keys, "wb");
        fopen(filename_values, "wb");
        free(filename_keys);
        free(filename_values);
    }
    free(component_id);

}

// allocate memory for lsm struct
// TODO: need to chck the validity of arguments (length of Cs_size ...)
// TODO: Make a generic constructor of lsm object
// (possible once the linked lists have been set up)
void init_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size){
    lsm->name = (char*)calloc(FILENAME_SIZE, sizeof(char));
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
        fclose(fkeys);
    }
    FILE *fvalues;
    if ((fvalues = fopen(filename_values, "rb")) == NULL) {
        fprintf(stderr, "can't open file %s \n", filename_values);
        exit(1);
    }
    else {
        fread(C->values, value_size, *Ne, fvalues);
        fclose(fvalues);
    }

    free(filename_keys);
    free(filename_values);
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

    free(filename_keys);
    free(filename_values);
}

// Function to write lsm tree to disk
// Need to write: CO, buffer and metadata (from struct)
void write_lsm_to_disk(LSM_tree *lsm){
    // Save memory components to disk
    write_disk_component(lsm->C0, lsm->name, VALUE_SIZE);
    write_disk_component(lsm->buffer, lsm->name, VALUE_SIZE);

    // Save metadata of the lsm to disk in file name.data
    char *filename = (char*) calloc(56, sizeof(char));
    sprintf(filename,"%s/meta.data", lsm->name);
    FILE* fout = fopen(filename, "wb");
    fwrite(lsm->name, sizeof(lsm->name), 1, fout);
    fwrite(&lsm->Ne, sizeof(int), 1, fout);
    fwrite(&lsm->Nc, sizeof(int), 1, fout);
    fwrite(lsm->Cs_Ne, sizeof(int), lsm->Nc+2, fout);
    fwrite(lsm->Cs_size, sizeof(int), lsm->Nc+2, fout);

    fclose(fout);
    free(filename);
}

// Try to read lsm from disk in its repository: name
void read_lsm_from_disk(LSM_tree *lsm, char *name){
    // Initialize lsm if needed
    if (lsm == NULL) lsm = (LSM_tree*) malloc(sizeof(LSM_tree));
    lsm->name = (char*)calloc(FILENAME_SIZE, sizeof(char));
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
    free(filename);

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

// Append (k,v) to the lsm tree
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
        merge_sort_with_values(lsm->C0->keys, lsm->C0->values, 0,
                               lsm->Cs_size[0]-1, VALUE_SIZE);
        // Case empty buffer
        if (lsm->buffer->Ne == 0){
            swap_component_pointer(lsm->C0, lsm->buffer);
        }
        else{
            // We don't free the memory in C0, we just update the number of
            // elements in it.
            merge_components(lsm->C0->keys, lsm->buffer->keys, lsm->C0->values,
                             lsm->buffer->values, lsm->C0->Ne, lsm->buffer->Ne,
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
    char * component_id = (char*) malloc(8*sizeof(char));

    // Check if current component is full
    while (lsm->Cs_Ne[current_C_index] >= lsm->Cs_size[current_C_index]){
        // Initialize next component
        component* next_component = (component *) malloc(sizeof(component));

        // Read next component from disk
        sprintf(component_id, "C%d", current_C_index);
        read_disk_component(next_component, lsm->name,
                            lsm->Cs_Ne + (current_C_index+1),
                            component_id,lsm->Cs_size + (current_C_index+1),
                            VALUE_SIZE);
        // Debug
        // printf("Before Merging\n");
        // printf("Ne in next Component: %d\n", *(next_component->Ne));
        if (next_component->Ne == 0){
            swap_component_pointer(current_component, next_component);
        }
        else{
            // We don't free the memory in prev component,
            // we just update the number of elements in it.
            merge_components(current_component->keys, next_component->keys,
                             current_component->values, next_component->values,
                             current_component->Ne, next_component->Ne,
                             VALUE_SIZE);
        }
        // Updates number of elements
        lsm->Cs_Ne[current_C_index + 1] += lsm->Cs_Ne[current_C_index];
        lsm->Cs_Ne[current_C_index] = 0;

        // Write the component to disk
        // TODO: write also done for the buffer (need to do it as a log)
        write_disk_component(next_component, lsm->name, VALUE_SIZE);
        write_disk_component(current_component, lsm->name, VALUE_SIZE);

        // Updates component
        if (current_C_index > 1){
            free_component(current_component);
            //current_component = (component *) malloc(sizeof(component));
        }
        current_C_index++;
        current_component = next_component;
        // TOFIX: Issue with buffer (because the next line does not change the pointer
        // outside the function)
        // free(current_component->keys);
        // free(current_component->values);
        // Make current component point to next_component
        
    }
    if (current_C_index >1 ) free_component(current_component);
    free(component_id);
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
    if (*index != -1){
        // Check key not deleted
        if (*(pC->values + (*index)*VALUE_SIZE) == '!') *index = -2;
        else strcpy(value, pC->values + (*index)*VALUE_SIZE);
    }
}

// Read value of key in LSMTree lsm
// return NULL if key not present
char* read_lsm(LSM_tree *lsm, int key){
    int j = 0;
    // TO check the status: -2 deleted, -1 not found else found
    int* index = (int*) malloc(sizeof(int));
    // Initialization
    *index = -1;
    char* value_output = (char*) malloc(VALUE_SIZE*sizeof(char));
    // Linear scan in C0
    printf("Reading C0\n");
    for (int i=0; i < lsm->Cs_Ne[0]; i++){
        if (lsm->C0->keys[i] == key){
            // Check that's not a deleted key
            if (*(lsm->C0->values + i*VALUE_SIZE) == '!'){
                *index = -2;
                break;
            } 
            printf("Found in C0\n");
            strcpy(value_output, lsm->C0->values + i*VALUE_SIZE);
            *index = 1;
        }
    }

    // Binary search over disk components (buffer and disk components) 
    component* pC;
    if (*index == -1){
        char * component_id = (char*) malloc(8*sizeof(char));
        pC = lsm->buffer;
        for (j=1; j<lsm->Nc+1; j++){
            if (lsm->Cs_Ne[j] > 0){
                // Case disk component: need to read the component from disk
                if (j != 1){
                    sprintf(component_id, "C%d", j-1);
                    read_disk_component(pC, lsm->name, lsm->Cs_Ne + j,
                                        component_id, lsm->Cs_size + j,
                                        VALUE_SIZE);
                }
                printf("Reading %s\n", pC->component_id);
                component_search(pC, key, index, value_output);
                // Key found (can still be deleted)
                if (*index != -1){
                    printf("Key Found in %s\n", pC->component_id);
                    // Case disk component
                    if (j>1) free_component(pC);
                    break;
                }
                // Free and reinitialize component for the next search
                if (j>1) {
                    free_component(pC);
                    pC = (component *) malloc(sizeof(component));
                }
            }
            // Initializing the component pointer for the first disk component
            if (j==1) pC = (component *) malloc(sizeof(component));
        }
        free(component_id);
    }
    // Check needed not to free the buffer 
    if ((j >= 2) && (*index < 0)){
        free(pC);
    }
    if (*index >= 0){
        free(index);
        return value_output;
    }
    free(value_output);
    free(index);
    // Case value not found: component was initialized only
    return NULL;
}

// Update (key, value) to the lsm tree: idea is to scan linearly
// C0 and update directly (key,value) if found, else append it; the
// update will occur when merging (merge keep always the key in the
// smallest component)
void update_lsm(LSM_tree *lsm, int key, char *value){
    // TODO: add bloom filter check
    // Linear scan of C0
    for (int i=0; i < lsm->Cs_Ne[0]; i++){
        if (lsm->C0->keys[i] == key){
            // Update the value for key i
            strcpy(lsm->C0->values + i*VALUE_SIZE, value);
            return ;
        }
    }
    // Append the update
    // TODO: correct update of the number of elements in the lsm tree
    // or decide if we want to have it as exact value
    append_lsm(lsm, key, value);
}

// Delete (key, value) to the lsm tree: idea is to scan linearly
// C0 and update directly (old_k,v) if found, else append it; the
// update will occur when merging (merge keep always the key in the
// smallest component)
void delete_lsm(LSM_tree *lsm, int key){
    // TODO: add bloom filter check
    // Linear scan of C0
    for (int i=0; i < lsm->Cs_Ne[0]; i++){
        if (lsm->C0->keys[i] == key){
            // check that key is not the last one
            if (i != lsm->Cs_Ne[0] - 1){
                // Switch the (k,v) tuple found with the last one appended
                lsm->C0->keys[i] = lsm->C0->keys[lsm->Cs_Ne[0] - 1];
                strcpy(lsm->C0->values + i*VALUE_SIZE,
                       lsm->C0->values + (lsm->Cs_Ne[0] - 1)*VALUE_SIZE);
            }
            // Update the count
            lsm->Cs_Ne[0]--;
            lsm->Ne--;
            return ;
        }
    }
    // Append the deletion (identified with the value: )
    // TODO: correct update of the number of elements in the lsm tree
    char* deletion = (char*) malloc(VALUE_SIZE*sizeof(char));
    sprintf(deletion, "!");
    append_lsm(lsm, key, deletion);
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
    int Nc = 6;
    int size_test = 1000000;
    // List contains Nc+2 elements: [C0, buffer, C1, C2,...]
    // Last component with very big size (as finite number of components chosen)
    int Cs_size[] = {SIZE, 3*SIZE, 9*SIZE, 27*SIZE, 81*SIZE, 729*SIZE, 9*729*SIZE, 1000000000};

    LSM_tree *lsm = (LSM_tree*) malloc(sizeof(LSM_tree));
    build_lsm(lsm, name, Nc, Cs_size);

    // filling the lsm
    char value[VALUE_SIZE];

    // Sorted keys
    // for (int i=0; i < size_test; i++){
    //     // Filling value
    //     sprintf(value, "hello%d", i%150);
    //     append_lsm(lsm, i, value);
    // }    

    // Unsorted keys
    // adding even keys
    int j;
    for (int i=0; i < size_test/2; i++){
        j = size_test/2 - i - 1;
        // Filling value
        sprintf(value, "hello%d", (2*j)%150);
        append_lsm(lsm, (2*j), value);
    }
    // adding odd keys
    for (int i=size_test/2; i < size_test; i++){
        // Filling value
        sprintf(value, "hello%d", (2*(i-size_test/2) + 1)%150);
        append_lsm(lsm, 2*(i-size_test/2) + 1, value);
    }

    // printf("Read after append: %s\n", lsm.file_components);
    // printf("Read after append: %s\n", lsm.file_components + FILENAME_SIZE);

    printf("Number of elements in LSMTree: %d\n", lsm->Ne);
    printf("Number of elements in C0: %d / %d\n", lsm->Cs_Ne[0], lsm->Cs_size[0]);
    printf("Number of elements in buffer: %d / %d\n", lsm->Cs_Ne[1], lsm->Cs_size[1]);
    for (int i=2; i<Nc+2; i++) printf("Number of elements in C%d: %d / %d\n",i-1, lsm->Cs_Ne[i], lsm->Cs_size[i]);

    // Print sequence of keys
    printf("First 10 keys of the buffer are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm->buffer->keys[i]);
    }
    printf("Last 10 keys of the buffer are\n");
    for (int i=*(lsm->buffer->Ne) - 10; i < *(lsm->buffer->Ne); i++){
        printf("%d \n", lsm->buffer->keys[i]);
    }
    printf("First 10 keys of CO are\n");
    for (int i=0; i < 10; i++){
        printf("%d \n", lsm->C0->keys[i]);
    }

    // Printing first element of disk component (read from disk)
    int disk_index = 5;
    char * comp_id = (char *) malloc(10*sizeof(char));
    sprintf(comp_id, "C%d", disk_index);

    component * C = (component *) malloc(sizeof(component));
    read_disk_component(C, name, lsm->Cs_Ne + disk_index+1, comp_id, lsm->Cs_size + disk_index+1, VALUE_SIZE);

    // Printing
    printf("First 10 keys of %s are\n", comp_id);
    for (int i=0; i < 10; i++){
        printf("%d \n", C->keys[i]);
    
    }

    // Reading from lsm
    int length = 5;
    int targets[] = {-3, 0, 100, 7000, 60000};
    char* value_read;
    for (int i=0; i<length; i++){
        value_read = read_lsm(lsm, targets[i]);
        if (value_read != NULL) printf("Reading key: %d; value found: %s\n", targets[i], value_read);
        else printf("Reading key: %d; key not found:\n", targets[i]);
        free(value_read);
    } 

    // Writing to disk
    write_lsm_to_disk(lsm);

    // Free memory
    free(comp_id);
    free_component(C);
    free_lsm(lsm);
}


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