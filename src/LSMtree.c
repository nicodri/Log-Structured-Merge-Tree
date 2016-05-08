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
    if (BLOOM_ON) lsm->bloom = (bloom_filter_t *) malloc(sizeof(bloom_filter_t));
}

// Create LSM Tree with a fixed number of component Nc without
// initialization of the components (on memory & on disk)
void create_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size,
              int filename_size){
    // Init struct lsm
    init_lsm(lsm, name, filename_size);

    // Init the bloom filter
    if (BLOOM_ON){
        // Bloom filter initialized with enough bits for the last component
        bloom_init(lsm->bloom, (uint64_t) BLOOM_SIZE, HASHES);
    }

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
    if (BLOOM_ON) bloom_destroy(lsm->bloom);
    free(lsm);
}

// Create LSM Tree with a fixed number of component Nc with
// initialization of the components on memory & on disk (create
// the files inside the folder).
// Check if folder exists and clean it if needed.
// Save metadata and memory component for recovery.
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

    // Save intialized state of the lsm
    write_lsm_to_disk(lsm);
}

// Function to write lsm tree to disk
// Need to write: CO, buffer and metadata (from struct)
// Metadata contain:
//     - name
//     - Ne
//     - Nc
//     - Cs_size
//     - Cs_Ne (updated regularly)

void write_lsm_to_disk(LSM_tree *lsm){
    // Save memory components to disk
    write_disk_component(lsm->C0, lsm->name, lsm->value_size, lsm->filename_size);
    write_disk_component(lsm->buffer, lsm->name, lsm->value_size, lsm->filename_size);

    // Save metadata of the lsm to disk in file name.data
    char *filename = (char*) calloc(56, sizeof(char));
    sprintf(filename,"%s/meta.data", lsm->name);
    FILE* fout = fopen(filename, "wb");
    printf("name to save %s\n", lsm->name);
    fwrite(lsm->name, sizeof(char), lsm->filename_size, fout);
    fwrite(&lsm->Ne, sizeof(int), 1, fout);
    fwrite(&lsm->Nc, sizeof(int), 1, fout);
    fwrite(&lsm->value_size, sizeof(int), 1, fout);
    fwrite(lsm->Cs_size, sizeof(int), lsm->Nc+2, fout);
    fwrite(lsm->Cs_Ne, sizeof(int), lsm->Nc+2, fout);
    // save bloom filter if enabled
    if (BLOOM_ON){
        fwrite(&lsm->bloom->size, sizeof(index_t), 1, fout);
        fwrite(&lsm->bloom->count, sizeof(index_t), 1, fout);
        fwrite(lsm->bloom->table, sizeof(index_t), (lsm->bloom->size) / 8, fout);
    }
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
    fread(lsm->name, sizeof(char), lsm->filename_size, fin);
    // TODO: assertion on name
    fread(&lsm->Ne, sizeof(int), 1, fin);
    fread(&lsm->Nc, sizeof(int), 1, fin);
    fread(&lsm->value_size, sizeof(int), 1, fin);
    lsm->Cs_Ne = (int *) malloc((lsm->Nc + 2)*sizeof(int));
    lsm->Cs_size = (int *) malloc((lsm->Nc + 2)*sizeof(int));
    fread(lsm->Cs_size, sizeof(int), lsm->Nc + 2, fin);
    fread(lsm->Cs_Ne, sizeof(int), lsm->Nc + 2, fin);
    if (BLOOM_ON){
        index_t* size = (index_t *) malloc(sizeof(index_t));
        fread(size, sizeof(index_t), 1, fin);
        bloom_init(lsm->bloom, *size, HASHES);
        fread(&lsm->bloom->count, sizeof(index_t), 1, fin);
        fread(lsm->bloom->table, sizeof(index_t), (lsm->bloom->size) / 8, fin);
        free(size);
    }
    fclose(fin);
    free(filename);

    // Read memory components from disk
    read_disk_component(lsm->C0, lsm->name, lsm->Cs_Ne, "C0", lsm->Cs_size, lsm->value_size,
                        filename_size);
    read_disk_component(lsm->buffer, lsm->name, lsm->Cs_Ne + 1, "buffer",
                        lsm->Cs_size + 1, lsm->value_size, filename_size);
}

// Append (k,v) to the lsm tree
// TODO: efficient log which saves the state of the LSM-tree
void append_lsm(LSM_tree *lsm, int key, char *value){
    // Track the number of appends to manage the logging of C0
    static int num_append = 0;
    num_append++;

    // Append to C0 on memory
    lsm->C0->keys[lsm->Cs_Ne[0]] = key;
    strcpy(lsm->C0->values + lsm->Cs_Ne[0]*lsm->value_size, value);

    //Increment number of elements in C0
    lsm->Cs_Ne[0]++;

    // Append to C0 on disk every k appends (for log purpose in cash of crash)
    // TODO: log on disk before update on memory?
    if (num_append % CO_TOLERANCE == 0){
        if (CO_TOLERANCE <= *lsm->Cs_size){
            // printf("Num append to write %d\n", num_append);
            // Bulk-append to C0
            append_on_disk(lsm->C0, num_append, lsm->name, lsm->value_size, lsm->filename_size);
            update_component_size(lsm);
        }
        // Re-initialize the appends counter
        num_append = 0;
    }

    // MERGING OPERATIONS
    
    // Check if C0 is full
    if (lsm->Cs_Ne[0] >= lsm->Cs_size[0]){
        // Inplace sorting of C0->keys and corresponding reorder in C0->values
        merge_sort_with_values(lsm->C0->keys, lsm->C0->values, 0,
                               lsm->Cs_size[0]-1, lsm->value_size);
        merge_components(lsm->buffer, lsm->C0, lsm->name, lsm->value_size, lsm->filename_size);

        // Update the number of elements in component on disk
        update_component_size(lsm);
    }

    // iterative over all the full components
    // First starts with buffer -> C1, then C1 -> C2 ,...
    int current_C_index = 1;
    component* current_component = lsm->buffer;
    char * component_id = (char*) malloc(8*sizeof(char));

    // TODO: case of the last component (only merging and reallocation of memory if needed)
    // Check if current component is still able to recieve one batch of its previous
    // component, else it's considered full and need to be flush in its next component
    while (lsm->Cs_Ne[current_C_index] + lsm->Cs_size[current_C_index-1] > lsm->Cs_size[current_C_index]){
        // Initialize and read next component
        component* next_component = (component *) malloc(sizeof(component));
        sprintf(component_id, "C%d", current_C_index);
        read_disk_component(next_component, lsm->name,
                            lsm->Cs_Ne + (current_C_index+1),
                            component_id,lsm->Cs_size + (current_C_index+1),
                            lsm->value_size, lsm->filename_size);
        merge_components(next_component, current_component, lsm->name, lsm->value_size,
                         lsm->filename_size);

        // Update the number of elements in component on disk
        update_component_size(lsm);

        // Updates component
        if (current_C_index > 1){
            free_component(current_component);
        }
        current_C_index++;
        current_component = next_component;
    }
    // To avoiding freeing the buffer
    if (current_C_index >1 ) free_component(current_component);;
    free(component_id);
}

// Insert key,value in lsm
// Wrapper for the append function just to update the total number
// of elements in the LSMTree (assuming a correct behavior of the user,
// i.e. insertion of new elements and updates/deletes of stored elts)
void insert_lsm(LSM_tree *lsm, int key, char *value){
    // increment total number of elements in the LSMTree
    lsm->Ne++;
    // Insert to the bloom filter
    if (BLOOM_ON) bloom_add(lsm->bloom, (uint64_t) key);
    append_lsm(lsm, key, value);
}

// Read value of key in LSMTree lsm
// return -1 if value not present, else index >=0 with value pointer
// set to the value found
int read_lsm(LSM_tree *lsm, int key, char* value){
    // Memory allocation
    int* index = (int*) malloc(sizeof(int)); // -1 not found else found
    if (value == NULL) value = (char*) malloc(lsm->value_size*sizeof(char));

    // Bloom filter check
    if (BLOOM_ON && bloom_check(lsm->bloom, (uint64_t) key) == 0){
        free(index);
        if (VERBOSE) printf("Bloom check on for key %d\n", key);
        return -1;
    }

    // Linear scan in C0 (initialize index to -1)
    keys_linear_search(index, key, lsm->C0->keys, lsm->Cs_Ne[0]);
    if (*index != -1){
        if (VERBOSE == 1) printf("Key found in C0\n");
        strcpy(value, lsm->C0->values + (*index)*lsm->value_size);
    }

    // Reading buffer
    // Checking extreme of the buffer
    //if ((*index == -1) && (key >= lsm->buffer->keys[0]) && (key <= lsm->buffer->keys[lsm->Cs_Ne[1]-1])){
    if (*index == -1){
        *index = binary_search(lsm->buffer->keys, key, 0, lsm->Cs_Ne[1]-1);
        if (*index != -1){
            if (VERBOSE == 1) printf("Key found in %s\n", lsm->buffer->component_id);
            strcpy(value, lsm->buffer->values + (*index)*lsm->value_size);
        }
    }

    // Binary search over disk components
    if (*index == -1){
        char* filename_keys = (char *) calloc(lsm->filename_size + 8,sizeof(char));

        // Starting with C1 (indexed at 2 in Cs_Ne)
        for (int j=2; j<lsm->Nc+1; j++){
            if (lsm->Cs_Ne[j] > 0){
                // Build filename of the keys
                get_files_name_disk(filename_keys, lsm->name, j-1, "k",
                                    lsm->filename_size);
                // Searching in component
                component_search(index, key, lsm->Cs_Ne[j], filename_keys);

                // Key found (can still be deleted)
                if (*index != -1){
                    if (VERBOSE == 1) printf("Key Found in C%d\n", j-1);
                    read_value(value, *index, lsm->name, j-1, 
                               lsm->value_size, lsm->filename_size);
                    break;
                }
            }
        }
        free(filename_keys);
    }
    // Check if key found and not previously deleted
    if ((*index >= 0) && (*(value) != *TOMBSTONE)){
        free(index);
        return 1;
    }
    free(index);
    // Case value not found: component was initialized only
    return -1;
}


// Read value of key in LSMTree lsm
// return -1 if value not present, else index >=0 with value pointer
// set to the value found
int read_lsm_parallel(LSM_tree *lsm, int key, char* value){
    // Memory allocation
    int* index = (int*) malloc(sizeof(int)); // -1 not found else found
    if (value == NULL) value = (char*) malloc(lsm->value_size*sizeof(char));

    // Linear scan in C0 (initialize index to -1)
    keys_linear_search(index, key, lsm->C0->keys, lsm->Cs_Ne[0]);
    if (*index != -1){
        if (VERBOSE == 1) printf("Key found in C0\n");
        strcpy(value, lsm->C0->values + (*index)*lsm->value_size);
    }

    // Reading buffer
    // Checking extreme of the buffer
    if ((*index == -1) && (key >= lsm->buffer->keys[0]) && (key <= lsm->buffer->keys[lsm->Cs_Ne[1]-1])){
        *index = binary_search(lsm->buffer->keys, key, 0, lsm->Cs_Ne[1]-1);
        if (*index != -1){
            if (VERBOSE == 1) printf("Key found in %s\n", lsm->buffer->component_id);
            strcpy(value, lsm->buffer->values + (*index)*lsm->value_size);
        }
    }

    // Binary search over disk components
    if (*index == -1){
        int result_code;
        void *thread_result;
        pthread_t threads[lsm->Nc-1];
        // Initialize the semaphore and the shared variable (gt any level)
        mutex = (sem_t*) malloc(sizeof(sem_t));
        mutex = sem_open("/mysemaphore", O_CREAT, 1);

        // Create common arguments of the threads
        arg_thread_common* common = (arg_thread_common*) malloc(sizeof(arg_thread_common) + sizeof(int));
        common->key = key;
        common->filename_size = lsm->filename_size;
        common->name = lsm->name;
        common->shared_level = (int *) malloc(sizeof(int));
        *(common->shared_level) = lsm->Nc + 3;
        // printf("Shared level init %d\n", *(common->shared_level));

        // V1: Associate signal with handler
        // struct sigaction actions;
        // memset(&actions, 0, sizeof(actions));
        // sigemptyset(&actions.sa_mask);
        // actions.sa_flags = 0;
        // actions.sa_handler = handler;

        // int rc = sigaction(SIGALRM,&actions,NULL);
        // V2: Associate signal with handler
        // signal(SIGUSR1, handler);

        // Starting with C1 (indexed at 2 in Cs_Ne)
        for (int j=2; j<lsm->Nc+1; j++){
            if (lsm->Cs_Ne[j] > 0){
                // Build the thread argument
                arg_thread* arg = malloc(sizeof(arg_thread) + sizeof(arg_thread_common*));
                // j starts at 2, we start the thread id at 0
                arg->thread_id = j-1;
                arg->Cs_Ne = lsm->Cs_Ne[j];
                arg->common = common;
                // Debugging
                // printf("thread_id: %d, key: %d, Cs_Ne: %d , lsmt name: %s, filename size %d\n",
                //        arg->thread_id, common->key, arg->Cs_Ne, common->name,
                //        common->filename_size);

                // Searching in component in current thread
                result_code = pthread_create(&threads[j], NULL, component_search_parallel,
                                             (void *) arg);
            }
        }
        // Reading answer from threads
        for (int j=2; j<lsm->Nc+1; j++){
            if (lsm->Cs_Ne[j] > 0){
                // If value was found by previous, next thread is cancelled
                if (*index != -1) {
                    // TOFIX: Wait for the thread to finish
                    pthread_join(threads[j], &thread_result);
                    // pthread_kill(threads[j], SIGALRM);
                    // pthread_detach(threads[j]);
                    continue;
                }
                result_code = pthread_join(threads[j], &thread_result);
                // Debugging
                // printf("Thread id: %d, result: %d\n", j-2, (int) thread_result);
                // Case key found
                if ((int) thread_result != -1){
                    // TOFIX: inefficient
                    *index = (int) thread_result;
                    // Key found (can still be deleted)
                    if (VERBOSE == 1) printf("Key Found in C%d, by thread id: %d\n", j-1, j-2);
                    read_value(value, *index, lsm->name, j-1,
                               lsm->value_size, lsm->filename_size);
                }
            }
        }
        // Freeing memory
        // free(common->shared_level);
        // free(common);
    }
    // Check if key found and not previously deleted
    if ((*index >= 0) && (*(value) != *TOMBSTONE)){
        free(index);
        return 1;
    }
    free(index);
    // Case value not found
    return -1;
}

// Update (key, value) to the lsm tree: idea is to scan linearly
// C0 and update directly (key,value) if found, else append it; the
// update will occur when merging (merge keep always the key in the
// smallest component)
void update_lsm(LSM_tree *lsm, int key, char *value){
    // Bloom filter check
    if (BLOOM_ON && bloom_check(lsm->bloom, (uint64_t) key) == 0){
        printf("UPDATE: Key %d was no present\n", key);
        return;
    }
    // Linear scan of C0
    int* index = (int*) malloc(sizeof(int));
    keys_linear_search(index, key, lsm->C0->keys, lsm->Cs_Ne[0]);
    if (*index != -1){
        // Update the value for key index
        strcpy(lsm->C0->values + (*index)*lsm->value_size, value);
    }
    else{
        // Append the update
        // TODO: correct update of the number of elements in the lsm tree
        // or decide if we want to have it as exact value
        append_lsm(lsm, key, value);
    }
    // free memory
    free(index);
}

// Delete (key, value) to the lsm tree: 
void delete_lsm(LSM_tree *lsm, int key){
    // Decrement total number of elments in lsm
    lsm->Ne--;
    // Use deletion character
    char* deletion = (char*) malloc(lsm->value_size*sizeof(char));
    sprintf(deletion, TOMBSTONE);
    update_lsm(lsm, key, deletion);
}

// Updates number of elements in each component in the metadata of the LSM
void update_component_size(LSM_tree *lsm){
    char *filename = (char*) calloc(56, sizeof(char));
    sprintf(filename,"%s/meta.data", lsm->name);
    FILE* fout = fopen(filename, "r+b");
    // Goto the offset of Cs_Ne
    fseek(fout, -(lsm->Nc+2)*sizeof(int), SEEK_END);
    fwrite(lsm->Cs_Ne, sizeof(int), lsm->Nc+2, fout);
    fclose(fout);
    free(filename);    
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