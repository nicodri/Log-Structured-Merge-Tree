#include "LSMtree.h"

// allocate memory for lsm struct
// TODO: need to chck the validity of arguments (length of Cs_size ...)
// TODO: Make a generic constructor of lsm object
// (possible once the linked lists have been set up)
void init_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size,
              int filename_size){
    lsm->name = (char*)calloc(filename_size, sizeof(char));
    strcpy(lsm->name, name);
    lsm->buffer = (component *) malloc(sizeof(component));
    lsm->C0 = (component *) malloc(sizeof(component));
    lsm->value_size = value_size;
    lsm->filename_size = filename_size;
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

// LSM destructor
void free_lsm(LSM_tree *lsm){
    free(lsm->name);
    free(lsm->Cs_Ne);
    free(lsm->Cs_size);
    free_component(lsm->C0);
    free_component(lsm->buffer);
    free(lsm);
}

// Initialize LSM_tree object with metadata and C0
// name need to be at most 7 car int
// TODO: check the validity of the args (ratios, Csize)
void build_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size,
               int filename_size){
    // allocate memory for lsm tree
    init_lsm(lsm, name, Nc, Cs_size, value_size, filename_size);

    // Initialize C0 and buffer
    init_component(lsm->C0, Cs_size, value_size, lsm->Cs_Ne, "C0");
    init_component(lsm->buffer, Cs_size + 1, value_size, lsm->Cs_Ne + 1,  "buffer");

    // Initialize on disk the disk components
    create_disk_component(name, Nc, filename_size);
}

// Function to write lsm tree to disk
// Need to write: CO, buffer and metadata (from struct)
// Metadata contain:
//     - name
//     - Ne
//     - Nc
//     - Cs_Ne
//     - Cs_size

void write_lsm_to_disk(LSM_tree *lsm){
    // Save memory components to disk
    write_disk_component(lsm->C0, lsm->name, lsm->value_size, lsm->filename_size);
    write_disk_component(lsm->buffer, lsm->name, lsm->value_size, lsm->filename_size);

    // Save metadata of the lsm to disk in file name.data
    char *filename = (char*) calloc(56, sizeof(char));
    sprintf(filename,"%s/meta.data", lsm->name);
    FILE* fout = fopen(filename, "wb");
    fwrite(lsm->name, sizeof(lsm->name), 1, fout);
    fwrite(&lsm->Ne, sizeof(int), 1, fout);
    fwrite(&lsm->Nc, sizeof(int), 1, fout);
    fwrite(&lsm->value_size, sizeof(int), 1, fout);
    fwrite(lsm->Cs_Ne, sizeof(int), lsm->Nc+2, fout);
    fwrite(lsm->Cs_size, sizeof(int), lsm->Nc+2, fout);

    fclose(fout);
    free(filename);
}

// Try to read lsm from disk in its repository: name
void read_lsm_from_disk(LSM_tree *lsm, char *name, int filename_size){
    // Initialize lsm if needed
    if (lsm == NULL) lsm = (LSM_tree*) malloc(sizeof(LSM_tree));
    lsm->name = (char*)calloc(filename_size, sizeof(char));
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
    fread(&lsm->value_size, sizeof(int), 1, fin);
    lsm->Cs_Ne = (int *) malloc((lsm->Nc + 2)*sizeof(int));
    lsm->Cs_size = (int *) malloc((lsm->Nc + 2)*sizeof(int));
    fread(lsm->Cs_Ne, sizeof(int), lsm->Nc + 2, fin);
    fread(lsm->Cs_size, sizeof(int), lsm->Nc + 2, fin);

    fclose(fin);
    free(filename);

    // Read memory components from disk
    read_disk_component(lsm->C0, lsm->name, lsm->Cs_Ne, "C0", lsm->Cs_size, lsm->value_size,
                        filename_size);
    read_disk_component(lsm->buffer, lsm->name, lsm->Cs_Ne + 1, "buffer",
                        lsm->Cs_size + 1, lsm->value_size, filename_size);
}

// Append (k,v) to the lsm tree
void append_lsm(LSM_tree *lsm, int key, char *value){
    // TODO: check validity of the args (size of the value, ...)

    // Append to C0
    lsm->C0->keys[lsm->Cs_Ne[0]] = key;
    strcpy(lsm->C0->values + lsm->Cs_Ne[0]*lsm->value_size, value);
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
                               lsm->Cs_size[0]-1, lsm->value_size);
        // Case empty buffer
        if (lsm->buffer->Ne == 0){
            swap_component_pointer(lsm->C0, lsm->buffer, lsm->value_size);
        }
        else{
            // We don't free the memory in C0, we just update the number of
            // elements in it.
            merge_components(lsm->C0->keys, lsm->buffer->keys, lsm->C0->values,
                             lsm->buffer->values, lsm->C0->Ne, lsm->buffer->Ne,
                             lsm->value_size);
        }
        // Updates number of elements
        lsm->Cs_Ne[1] += lsm->Cs_Ne[0];
        lsm->Cs_Ne[0] = 0;

        // Updating log on disk
        write_disk_component(lsm->C0, lsm->name, lsm->value_size, lsm->filename_size);
        write_disk_component(lsm->buffer, lsm->name, lsm->value_size, lsm->filename_size);
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
                            lsm->value_size, lsm->filename_size);
        // Debug
        // printf("Before Merging\n");
        // printf("Ne in next Component: %d\n", *(next_component->Ne));
        if (next_component->Ne == 0){
            swap_component_pointer(current_component, next_component, lsm->value_size);
        }
        else{
            // We don't free the memory in prev component,
            // we just update the number of elements in it.
            merge_components(current_component->keys, next_component->keys,
                             current_component->values, next_component->values,
                             current_component->Ne, next_component->Ne,
                             lsm->value_size);
        }
        // Updates number of elements
        lsm->Cs_Ne[current_C_index + 1] += lsm->Cs_Ne[current_C_index];
        lsm->Cs_Ne[current_C_index] = 0;

        // Write the component to disk
        // TODO: write also done for the buffer (need to do it as a log)
        write_disk_component(next_component, lsm->name, lsm->value_size, lsm->filename_size);
        write_disk_component(current_component, lsm->name, lsm->value_size, lsm->filename_size);

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

// Read value of key in LSMTree lsm
// return NULL if key not present
char* read_lsm(LSM_tree *lsm, int key){
    int j = 0;
    // TO check the status: -2 deleted, -1 not found else found
    int* index = (int*) malloc(sizeof(int));
    // Initialization
    *index = -1;
    char* value_output = (char*) malloc(lsm->value_size*sizeof(char));
    // Linear scan in C0
    if (VERBOSE == 1) printf("Reading C0\n");
    for (int i=0; i < lsm->Cs_Ne[0]; i++){
        if (lsm->C0->keys[i] == key){
            // Check that's not a deleted key
            if (*(lsm->C0->values + i*lsm->value_size) == '!'){
                *index = -2;
                break;
            } 
            if (VERBOSE == 1) printf("Found in C0\n");
            strcpy(value_output, lsm->C0->values + i*lsm->value_size);
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
                                        lsm->value_size, lsm->filename_size);
                }
                if (VERBOSE == 1) printf("Reading %s\n", pC->component_id);
                component_search(pC, key, index, value_output, lsm->value_size);
                // Key found (can still be deleted)
                if (*index != -1){
                    if (VERBOSE == 1) printf("Key Found in %s\n", pC->component_id);
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
            strcpy(lsm->C0->values + i*lsm->value_size, value);
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
                strcpy(lsm->C0->values + i*lsm->value_size,
                       lsm->C0->values + (lsm->Cs_Ne[0] - 1)*lsm->value_size);
            }
            // Update the count
            lsm->Cs_Ne[0]--;
            lsm->Ne--;
            return ;
        }
    }
    // Append the deletion (identified with the value: )
    // TODO: correct update of the number of elements in the lsm tree
    char* deletion = (char*) malloc(lsm->value_size*sizeof(char));
    sprintf(deletion, "!");
    append_lsm(lsm, key, deletion);
}

// Print number of element in the tree
void print_state(LSM_tree *lsm){
    printf("State of the lsm: %s\n", lsm->name);
    printf("Number of elements in LSMTree: %d\n", lsm->Ne);
    printf("Number of elements in C0: %d / %d\n", lsm->Cs_Ne[0], lsm->Cs_size[0]);
    printf("Number of elements in buffer: %d / %d\n", lsm->Cs_Ne[1], lsm->Cs_size[1]);
    for (int i=2; i<lsm->Nc+2; i++) printf("Number of elements in C%d: %d / %d\n",i-1,
                                      lsm->Cs_Ne[i], lsm->Cs_size[i]);
}