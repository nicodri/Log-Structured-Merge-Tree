#include "LSMtree.h"

// Init first elements of an LSM_tree:
//  - name
//  - buffer
//  - C0
//  - filename_size (size of name)
//  - Ne (initliazed to 0)
void init_lsm(LSM_tree *lsm, char* name, int filename_size){
    lsm->name = (char*)calloc(filename_size, sizeof(char));
    strcpy(lsm->name, name);
    lsm->buffer = (component *) malloc(sizeof(component));
    lsm->C0 = (component *) malloc(sizeof(component));
    lsm->filename_size = filename_size;
    lsm->Ne = 0;

}

// Create LSM Tree with a fixed number of component Nc without
// initialization of the components (on memory & on disk)
void create_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size,
              int filename_size){
    // Init struct lsm
    init_lsm(lsm, name, filename_size);

    // List contains Nc+2 elements: [C0, buffer, C1, C2,...]
    lsm->value_size = value_size;
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

// Create LSM Tree with a fixed number of component Nc with
// initialization of the components on memory & on disk (create
// the files inside the folder, with a cleaning if needed).
// Check if folder exists and clean it if needed.
// TODO: check the validity of the args
void build_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size,
               int filename_size){
    // Allocate memory and create lsm
    create_lsm(lsm, name, Nc, Cs_size, value_size, filename_size);

    // Initialize C0 and buffer (on memory)
    init_component(lsm->C0, Cs_size, value_size, lsm->Cs_Ne, "C0");
    init_component(lsm->buffer, Cs_size + 1, value_size, lsm->Cs_Ne + 1,  "buffer");

    // Check if folder exists
    if (access(name, F_OK) == -1){
        printf("ERROR: folder name %s not present\n", name);
        return;
    }
    // Clean the folder: error printed if already cleaned
    char command[50];
    sprintf(command, "exec rm -rf %s/*", name);
    system(command);
    //free(command);

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
    // Init struct lsm
    init_lsm(lsm, name, filename_size);

    // Read meta.data
    char *filename = (char*) malloc(16*sizeof(char) + sizeof(name));
    sprintf(filename,"%s/meta.data", name);
    FILE* fin = fopen(filename, "rb");
    fread(lsm->name, sizeof(name), 1, fin);
    // TODO: assertion on name
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
// TOTEST: append_on_disk
// TODO: efficient log which saves the state of the LSM-tree
void append_lsm(LSM_tree *lsm, int key, char *value){
    // TODO: check validity of the args (size of the value, ...)

    // Before on disk than on memory (for log purpose in cash of crash)
    // Append to C0 on disk
    //append_on_disk(lsm->C0, key, value, lsm->name, lsm->value_size, lsm->filename_size);

    // Append to C0 on memory
    lsm->C0->keys[lsm->Cs_Ne[0]] = key;
    strcpy(lsm->C0->values + lsm->Cs_Ne[0]*lsm->value_size, value);

    //Increment number of elements
    lsm->Ne++;
    lsm->Cs_Ne[0]++;

    // MERGING OPERATIONS
    
    // Check if C0 is full
    if (lsm->Cs_Ne[0] >= lsm->Cs_size[0]){
        // Inplace sorting of C0->keys and corresponding reorder in C0->values
        merge_sort_with_values(lsm->C0->keys, lsm->C0->values, 0,
                               lsm->Cs_size[0]-1, lsm->value_size);
        merge_components(lsm->buffer, lsm->C0, lsm->name, lsm->value_size, lsm->filename_size);
    }

    // Step 2: iterative over all the full components
    // First starts with buffer -> C1, then C1 -> C2 ,...
    int current_C_index = 1;
    component* current_component = lsm->buffer;
    char * component_id = (char*) malloc(8*sizeof(char));

    // Check if current component is full
    while (lsm->Cs_Ne[current_C_index] >= lsm->Cs_size[current_C_index]){
        // Initialize and read next component
        component* next_component = (component *) malloc(sizeof(component));
        sprintf(component_id, "C%d", current_C_index);
        read_disk_component(next_component, lsm->name,
                            lsm->Cs_Ne + (current_C_index+1),
                            component_id,lsm->Cs_size + (current_C_index+1),
                            lsm->value_size, lsm->filename_size);
        merge_components(next_component, current_component, lsm->name, lsm->value_size, lsm->filename_size);

        // Updates component
        if (current_C_index > 1){
            free_component(current_component);
        }
        current_C_index++;
        current_component = next_component;
    }
    // To avoiding freeing the buffer
    if (current_C_index >1 ) free_component(current_component);
    free(component_id);
}

// Read value of key in LSMTree lsm
// return NULL if key not present
char* read_lsm(LSM_tree *lsm, int key){
    // Memory allocation
    int* index = (int*) malloc(sizeof(int)); // -1 not found else found
    char* value_output = (char*) malloc(lsm->value_size*sizeof(char));

    // Linear scan in C0 (initialize index to -1)
    keys_linear_search(index, key, lsm->C0->keys, lsm->Cs_Ne[0]);
    if (*index != -1){
        if (VERBOSE == 1) printf("Key found in C0\n");
        strcpy(value_output, lsm->C0->values + (*index)*lsm->value_size);
    }

    // Reading buffer
    // Checking extreme of the buffer
    if ((*index == -1) && (key >= lsm->buffer->keys[0]) && (key <= lsm->buffer->keys[lsm->Cs_Ne[1]-1])){
        *index = binary_search(lsm->buffer->keys, key, 0, lsm->Cs_Ne[1]-1);
        if (*index != -1){
            if (VERBOSE == 1) printf("Key found in %s\n", lsm->buffer->component_id);
            strcpy(value_output, lsm->buffer->values + (*index)*lsm->value_size);
        }
    }

    // Binary search over disk components
    if (*index == -1){
        char* filename_keys = (char *) calloc(lsm->filename_size + 8,sizeof(char));
        char * component_id = (char*) malloc(8*sizeof(char));

        // Starting with C1 (indexed at 2 in Cs_Ne)
        for (int j=2; j<lsm->Nc+1; j++){
            if (lsm->Cs_Ne[j] > 0){
                // Build filename of the keys
                sprintf(component_id, "C%d", j-1);
                get_files_name(filename_keys, lsm->name, component_id, "k", lsm->filename_size);
                // Searching in component
                component_search(index, key, lsm->Cs_Ne[j], filename_keys);

                // Key found (can still be deleted)
                if (*index != -1){
                    if (VERBOSE == 1) printf("Key Found in %s\n", component_id);
                    read_value(value_output, *index, lsm->name, component_id, 
                               lsm->value_size, lsm->filename_size);
                    break;
                }
            }
        }
        free(filename_keys);
        free(component_id);
    }
    // Check if key found and not previously deleted
    if ((*index >= 0) && (*(value_output) != '!')){
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
    int* index = (int*) malloc(sizeof(int));
    keys_linear_search(index, key, lsm->C0->keys, lsm->Cs_Ne[0]);
    if (*index != -1){
        // Update the value for key index
        strcpy(lsm->C0->values + (*index)*lsm->value_size, value);
        return ;
    }
    // Append the update
    // TODO: correct update of the number of elements in the lsm tree
    // or decide if we want to have it as exact value
    append_lsm(lsm, key, value);
}

// Delete (key, value) to the lsm tree: 
void delete_lsm(LSM_tree *lsm, int key){
    char* deletion = (char*) malloc(lsm->value_size*sizeof(char));
    sprintf(deletion, TOMBSTONE);
    update_lsm(lsm, key, deletion);
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