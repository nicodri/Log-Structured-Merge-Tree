#include "LSMTree.h"

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

// Create on disk the files for the disk component
void create_disk_component(char* name, int Nc, int filename_size){
    char *filename_keys, *filename_values;
    // Arbitrary size big enough
    char* component_id = (char*) malloc(16*sizeof(char));
    for (int i=1; i<Nc; i++){
        // Building filename
        sprintf(component_id,"C%d", i);
        filename_keys = get_files_name(name, component_id, "k", filename_size);
        filename_values = get_files_name(name, component_id, "v", filename_size);
        fopen(filename_keys, "wb");
        fopen(filename_values, "wb");
        free(filename_keys);
        free(filename_values);
    }
    free(component_id);
}

// TOFIX: need to initialized the files for each component before calling this function
void read_disk_component(component* C, char *name, int* Ne, char *component_id,
                         int* component_size, int value_size, int filename_size){
    // Initialize component
    init_component(C, component_size, value_size, Ne, component_id);
    // Building filename
    char *filename_keys = get_files_name(name, component_id, "k", filename_size);
    char *filename_values = get_files_name(name, component_id, "v", filename_size);

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

void write_disk_component(component *pC, char *name, int value_size, int filename_size){
    // Building filename
    char *filename_keys = get_files_name(name, pC->component_id, "k", filename_size);
    char *filename_values = get_files_name(name, pC->component_id, "v", filename_size);

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

// TODO: create external function to apply on both keys and values
// Swap the pointer to keys and values between the two components
// Assumes next_component->S > current_component->S
void swap_component_pointer(component *current_component, component *next_component,
                            int value_size){
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
    realloc(current_component->values, value_size * *(current_component->S) * sizeof(char));
    char *tmpv = realloc(next_component->values, value_size * *(next_component->S) * sizeof(char));
    if (tmpv == NULL)
    {
        // could not realloc, alloc new space and copy
        char *newv = (char *) malloc(value_size * *(next_component->S) * sizeof(char));
        for (int i=0; i < *(next_component->Ne) + *(current_component->Ne); i++){
            strcpy(newv + i*value_size, next_component->values + i*value_size);
        }
        free(next_component->values);
        next_component->values = newv;
    }
    else
    {
        next_component->values = tmpv;
    }
}

// Binary search of the key in component pC
// Set index to 1 and value to the value if index, else set index to 0.
void component_search(component *pC, int key, int* index, char* value, int value_size){
    *index = binary_search(pC->keys, key, 0, *(pC->Ne)-1);
    if (*index != -1){
        // Check key not deleted
        if (*(pC->values + (*index)*value_size) == '!') *index = -2;
        else strcpy(value, pC->values + (*index)*value_size);
    }
}