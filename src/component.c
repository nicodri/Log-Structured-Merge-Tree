#include "LSMTree.h"

// Component constructor
void init_component(component * c, int* component_size, int value_size, int* Ne,
                    char* component_id){
    // Valgrind modif: malloc to calloc (because of padding)
    c->keys = (int *) calloc((*component_size), sizeof(int));
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
    char *filename = (char *) calloc(filename_size + 8, sizeof(char));
    // Arbitrary size big enough
    char* component_id = (char*) malloc(16*sizeof(char));
    char component_type[] = {'k','v'};
    for (int i=1; i<=Nc; i++){
        sprintf(component_id,"C%d", i);
        for (int k=0; k<2; k++){
            // Building filename
            get_files_name(filename, name, component_id, &component_type[k], filename_size);
            fopen(filename, "wb");
        }
    }
    // Free memory
    free(filename);
    free(component_id);
}

// TOFIX: need to initialized the files for each component before calling this function
void read_disk_component(component* C, char *name, int* Ne, char *component_id,
                         int* component_size, int value_size, int filename_size){
    // Initialize component
    init_component(C, component_size, value_size, Ne, component_id);
    // Building filename
    char *filename_keys = (char *) calloc(filename_size + 8,sizeof(char));
    char *filename_values = (char *) calloc(filename_size + 8,sizeof(char));
    get_files_name(filename_keys, name, component_id, "k", filename_size);
    get_files_name(filename_values, name, component_id, "v", filename_size);

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

// Write on disk the keys and values of the component pC
void write_disk_component(component *pC, char *name, int value_size, int filename_size){
    // Building filename
    char *filename_keys = (char *) calloc(filename_size + 8,sizeof(char));
    char *filename_values = (char *) calloc(filename_size + 8,sizeof(char));
    get_files_name(filename_keys, name, pC->component_id, "k", filename_size);
    get_files_name(filename_values, name, pC->component_id, "v", filename_size);

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

// Append to a component on disk the last N keys/values 
void append_on_disk(component * C, int N, char* name, int value_size,
                    int filename_size){
    char *filename = (char *) calloc(filename_size + 8,sizeof(char));
    FILE* fd;
    // Check the number of keys in the component
    if (*C->Ne < N) N = *C->Ne;

    // Write the N last keys
    get_files_name(filename, name, C->component_id, "k", filename_size);
    fd = fopen(filename, "ab");
    fwrite(C->keys + (*C->Ne - N), sizeof(int), N, fd);
    fclose(fd);

    // Write the N last values
    get_files_name(filename, name, C->component_id, "v", filename_size);
    fd = fopen(filename, "ab");
    fwrite(C->values + (*C->Ne - N)*value_size, value_size*sizeof(char), N, fd);
    fclose(fd);

    // Free memory
    free(filename);
}

// Read value at given index in the component on disk
void read_value(char* value, int index, char* name, int component_index, int value_size,
                int filename_size){
    if (value == NULL) value = (char*) malloc(value_size*sizeof(char));
    char* filename = (char *) calloc(filename_size + 8,sizeof(char));
    // Reading the found value at the corresponding index
    get_files_name_disk(filename, name, component_index, "v",
                   filename_size);

    FILE* fd = fopen(filename, "rb");
    // Get the file descriptor
    // int fint = fileno(fd);
    // printf("File descriptor: %d\n", fint);
    if (fd == NULL){
        perror("fopen");
    }
    fseek(fd, index*value_size*sizeof(char), SEEK_SET);
    fread(value, value_size, 1, fd);
    fclose(fd);

    // free memory
    free(filename);
}

// TOFIX: code redundancy BUT hard to divide into 2 functions because different type
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
    // KEYS
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

    // VALUES
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

void merge_components(component* next_component, component* current_component, char* name,
                      int value_size, int filename_size){
    if (next_component->Ne == 0){
        swap_component_pointer(current_component, next_component, value_size);
    }
    else{
        // We don't free the memory in prev component,
        // we just update the number of elements in it.
        merge_list(current_component->keys, next_component->keys,
                   current_component->values, next_component->values,
                   current_component->Ne, next_component->Ne,
                   value_size);
    }
    // Updates number of elements
    *next_component->Ne += *current_component->Ne;
    *current_component->Ne = 0;

    // Write the component to disk
    write_disk_component(next_component, name, value_size, filename_size);
    write_disk_component(current_component, name, value_size, filename_size);
}

// Seach in the disk component stored in filename key.
void component_search(int* index, int key, int length, char* filename){
    // Mapping the file into memory
    int fd = open(filename, O_RDONLY);
    if (fd == -1){
        perror("open");
    }
    int* keys = mmap(0, length*sizeof(int), PROT_READ, MAP_SHARED, fd, 0);
    if (keys == MAP_FAILED){
        perror ("mmap");
    }

    // Checking extreme of the current component
    // if ((key >= keys[0]) && (key <= keys[length-1])){
    // Binary search
    if (VERBOSE == 1) printf("Reading %s\n", filename);
    *index = binary_search(keys, key, 0, length-1);                    
    // }

    // Closing file
    if (close(fd) == -1){
        perror ("close");
    }

    // Free mmap memory
    munmap(keys, length*sizeof(int));
}

void *component_search_parallel(void *argument){
    // Change the cancel state to asynchronous
    // pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int index = -1;
    // Reading argument
    arg_thread *arg = (arg_thread *) argument;
    arg_thread_common *common = (arg_thread_common*) arg->common;

    // Getting filename of the disk component
    char* filename = (char*) calloc(common->filename_size + 8,sizeof(char));
    if (filename == NULL) {
        fprintf(stderr, "failed to allocate memory.\n");
        pthread_exit( (void*) index);
    }
    get_files_name_disk(filename, common->name, arg->thread_id, "k", common->filename_size);

    // Printing arg
    // printf("Inside Thread id: %d, key: %d, Cs_Ne: %d, filename: %s\n", arg->thread_id,
    //        common->key, arg->Cs_Ne, filename);

    // Mapping the file into memory
    int fd = open(filename, O_RDONLY);
    // printf("THread: File descriptor: %d\n", fd);
    if (fd == -1){
        perror("open");
    }
    int* keys = mmap(0, arg->Cs_Ne*sizeof(int), PROT_READ, MAP_SHARED, fd, 0);
    if (keys == MAP_FAILED){
        perror ("mmap");
    }
    // Closing file (a creaf has been created by mmap)
    if (close(fd) == -1){
        perror ("close");
    }

    // Checking extreme of the current component
    if ((common->key >= keys[0]) && (common->key <= keys[arg->Cs_Ne-1])){
        // Binary search
        if (VERBOSE == 1) printf("Reading %s\n", filename);
        index = binary_search_signal(keys, common->key, 0, arg->Cs_Ne-1, arg->thread_id,
                                     common->shared_level, FREQUENCE);
        // update shared variable if key found
        if (index != -1){
            sem_wait(mutex);
            // printf("Semaphore locked. Shared level init %d thread id %d\n", *(common->shared_level), arg->thread_id);
            if (*common->shared_level > arg->thread_id) *common->shared_level = arg->thread_id;
            sem_post(mutex);
        }                    
    }

    // Free mmap memory
    munmap(keys, arg->Cs_Ne*sizeof(int));
    // Free thread_arg
    free(filename);
    free(arg);

    pthread_exit( (void*) index);

    // return (void *) (index);
}
