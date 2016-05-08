#include "LSMTree.h"

// Contain useful functions for the experiments, for example
// to populate an LSM tree, do batch reads/writes.

// Print an array of int
void print_array_int(int* array, int size){
    printf("Array is [ ");
    for (int i=0; i<size; i++) printf("%d ", array[i]);
    printf("]\n");
}

// Print an array of double
void print_array_double(double* array, int size){
    printf("Array is [ ");
    for (int i=0; i<size; i++) printf("%f ", array[i]);
    printf("]\n");
}

// Custom read of the values at the key
void read_test(LSM_tree* lsm, int key){
    char* value_read = (char*) malloc(lsm->value_size*sizeof(char));
    int check = read_lsm(lsm, key, value_read);
    if (check == 1){
        if (VERBOSE == 1) printf("Reading key: %d; value found: %s\n",
                                   key, value_read);
    }
    else {
        if (VERBOSE == 1) printf("Reading key: %d; key not found:\n", key);
    }
    free(value_read); 
}

void read_parallel_test(LSM_tree* lsm, int key){
    // Timing
    clock_t begin, end;
    double time_spent;
    begin = clock();

    char* value_read = (char*) malloc(lsm->value_size*sizeof(char));
    int check = read_lsm_parallel(lsm, key, value_read);
    if (check == 1){
        if (VERBOSE == 1) printf("Reading key: %d; value found: %s\n",
                                   key, value_read);
    }
    else {
        if (VERBOSE == 1) printf("Reading key: %d; key not found:\n", key);
    }
    free(value_read); 
    // Timing
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    if (VERBOSE == 1) printf("-------------- time: %f \n", time_spent);
}

// Generate a LSMT with num_elements (keys [0, num_elements[, value: 'aa..aa_{key%1000}' )
// arg: sorted too insert keys in sorted order or not.
// Return the execution time
double LSMTree_generation(char*name, int Nc, int* Cs_size, int value_size, int num_elements,
                        int sorted){
    int i;
    int* array;
    // if not sorted, generate an array of integers and shuffle it
    if (sorted == 0){
        // Initialize the seed
        srand(time(NULL));
        array = (int*) malloc(num_elements * sizeof(int));
        // Fill array
        for (i=0; i<num_elements; i++) array[i] = i;
        // Shuffle array
        for (i = 0; i < num_elements; i++) {
            int temp = array[i];
            int randomIndex = rand() % num_elements;
            array[i] = array[randomIndex];
            array[randomIndex] = temp;
        }
    }

    // Timing
    clock_t begin, end;
    double time_spent;
    begin = clock();
    printf("-------------------\n");
    printf("LSM TREE GENERATION: %d elements %d components\n", num_elements, Nc);

    char* value = (char*) malloc(value_size * sizeof(char));
    // sanity check
    if (value_size <= 12){
        printf("Too small value size %d\n", value_size);
        return -1;
    }
    else{
        // synthetic value
        for (i=0; i<value_size - 7; i++) value[i] = 'a';
    }

    // Create the tree
    LSM_tree *lsm = (LSM_tree*) malloc(sizeof(LSM_tree));
    build_lsm(lsm, name, Nc, Cs_size, value_size, FILENAME_SIZE);

    // Fill the tree:
    // Case sorted
    if (sorted == 1){
        for (i=0; i < num_elements; i++){
            // Filling value
            sprintf(value + value_size - 7, "_%d", i%10000);
            insert_lsm(lsm, i, value);
        }
    }
    // Case unsorted
    else{
        for (i=0; i < num_elements; i++){
            // Filling value
            sprintf(value + value_size - 7, "_%d", array[i]%10000);
            insert_lsm(lsm, array[i], value);
        }
        free(array);
    }
    // Sanity check
    print_state(lsm);

    // Writing to disk
    write_lsm_to_disk(lsm);

    // Free memory
    free_lsm(lsm);
    free(value);

    // Timing
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("time: %f \n", time_spent);
    return time_spent;
}
// UPDATE lsm tree with num_updates with key randomly sampled in the range [key_down, key_up]
// value is artificially built: 'bb..bb_{key%1000}'
// Return the execution time
double batch_updates(LSM_tree *lsm, int num_updates, int key_down, int key_up){
    // Timing
    clock_t begin, end;
    double time_spent;
    begin = clock();
    printf("-------------------\n");
    printf("BATCH UPDATES: %d\n",num_updates );
    printf("START %d\nEND: %d\n", key_down, key_up);

    // To store the value to update
    char* value = (char*) malloc(lsm->value_size * sizeof(char));
    int key, i;

    // LSMTree assumed already built => no sanity check
    for (i=0; i<lsm->value_size - 7; i++) value[i] = 'b';

    // Initialize the seed
    srand(time(NULL));

    for (i=0; i<num_updates; i++){
        // sampling a key in the range [key_down, key_up]
        key = (int)(key_down + (rand()/(float)RAND_MAX) * (key_up - key_down));
        // building an updated value
        sprintf(value + lsm->value_size - 7, "_%d", i%10000);
        update_lsm(lsm, key, value);
    } 

    // Free memory
    free(value);

    // Timing
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("time: %f \n", time_spent);

    return time_spent;
}

// READ lsm tree with num_reads with key randomly sampled in the range [key_down, key_up]
// Return the execution time
double batch_reads(LSM_tree *lsm, int num_reads, int key_down, int key_up){
    // Timing
    clock_t begin, end;
    double time_spent;
    begin = clock();
    printf("-------------------\n");
    printf("BATCH READS: %d\n", num_reads );
    printf("START %d\nEND: %d\n", key_down, key_up);

    // To store the value read
    char* value = (char*) malloc(lsm->value_size * sizeof(char));
    int key, i;

    // Initialize the seed
    srand(time(NULL));

    for (i=0; i<num_reads; i++){
        // sampling a key in the range [key_down, key_up]
        key = (int)(key_down + (rand()/(float)RAND_MAX) * (key_up - key_down));
        read_lsm(lsm, key, value);
    } 
    // Timing
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("time: %f \n", time_spent);

    // Free memory
    free(value);

    return time_spent;
}

// READ lsm tree with num_reads with key randomly sampled in the range [key_down, key_up]
// Return the execution time
double batch_parallel_reads(LSM_tree *lsm, int num_reads, int key_down, int key_up){
    // Timing
    clock_t begin, end;
    double time_spent;
    begin = clock();
    printf("-------------------\n");
    printf("BATCH READS: %d\n", num_reads );
    printf("START %d\nEND: %d\n", key_down, key_up);

    // To store the value read
    char* value = (char*) malloc(lsm->value_size * sizeof(char));
    int key, i;

    // Initialize the seed
    srand(time(NULL));

    for (i=0; i<num_reads; i++){
        // sampling a key in the range [key_down, key_up]
        key = (int)(key_down + (rand()/(float)RAND_MAX) * (key_up - key_down));
        read_lsm_parallel(lsm, key, value);
    } 
    // Free memory
    free(value);

    // Timing
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("time: %f \n", time_spent);

    return time_spent;
}