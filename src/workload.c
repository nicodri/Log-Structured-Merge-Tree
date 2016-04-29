#include "LSMTree.h"
#include <time.h>
#include <math.h>

#define FILENAME_SIZE 16

// ---------------------------------------------
// -------------------------- TEST ROUTINES
// ---------------------------------------------

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

// ---------------------------------------------
// -------------------------- WORKLOAD ROUTINES
// ---------------------------------------------

// Generate a LSMT with num_elements (keys [0, num_elements[, value: 'aa..aa_{key%1000}' )
// arg: sorted too insert keys in sorted order or not.
void LSMTree_generation(char*name, int Nc, int* Cs_size, int value_size, int num_elements,
                        int sorted){
    // Timing
    clock_t begin, end;
    double time_spent;
    begin = clock();
    printf("-------------------\n");
    printf("LSM TREE GENERATION: %d elements %d components\n", num_elements, Nc);

    char* value = (char*) malloc(value_size * sizeof(char));
    int i;
    // sanity check
    if (value_size <= 12){
        printf("Too small value size %d\n", value_size);
        return;
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

}

// UPDATE lsm tree with num_updates with key randomly sampled in the range [key_down, key_up]
// value is artificially built: 'bb..bb_{key%1000}'
void batch_updates(LSM_tree *lsm, int num_updates, int key_down, int key_up){
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
}

// READ lsm tree with num_reads with key randomly sampled in the range [key_down, key_up]
void batch_reads(LSM_tree *lsm, int num_reads, int key_down, int key_up){
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

}

// READ lsm tree with num_reads with key randomly sampled in the range [key_down, key_up]
void batch_parallel_reads(LSM_tree *lsm, int num_reads, int key_down, int key_up){
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

}

// TOFIX: Take as arguments:
//      - filename defining metadata of the LSM tree
int main(int argc, char *argv[]){
    printf("%d\n", argc);
    if (argc == 2) printf("Need to provide 1 filename as argument\n");

    else{
        // LSMT parameters
        int Nc = 6;
        int SIZE = 1000;
        int Cs_size[] = {SIZE, 3*SIZE, 9*SIZE, 27*SIZE, 81*SIZE, 729*SIZE, 9*729*SIZE, 1000000000};
        char name[] = "test";
        int num_elements = 1000000;
        int value_size = 32;

        // Case generation
        LSMTree_generation(name, Nc, Cs_size, value_size, num_elements, 1);

        // ---------------- TEST READING LSM FROM DISK
        printf("Reading LSM from disk:\n");
        LSM_tree *lsm_backup = (LSM_tree *)malloc(sizeof(LSM_tree));
        read_lsm_from_disk(lsm_backup, name, FILENAME_SIZE);
        print_state(lsm_backup);

        // Read test
        read_test(lsm_backup, 100);
        read_test(lsm_backup, 1000);
        read_test(lsm_backup, 10000);
        read_test(lsm_backup, 100000);
        read_test(lsm_backup, 999999);
        read_test(lsm_backup, 1000000);

        // -------------- bulk updates
        // int num_updates = 10000;
        // equilibrated
        // batch_updates(lsm_backup, num_updates, 0, num_elements);
        
        // Skewed (at the end)
        // batch_updates(lsm_backup, num_updates, (int) (0.8 * (float) num_elements), num_elements);

        // -------------- bulk reads
        // int num_reads = 10000;
        // // equilibrated
        // batch_reads(lsm_backup, num_reads, 0, num_elements);
        
        // // Skewed (at the end)
        // batch_reads(lsm_backup, num_reads, (int) (0.8 * (float) num_elements), num_elements);

        // // Skewed (with half outside)
        // batch_reads(lsm_backup, num_reads, (int) (0.9 * (float) num_elements),
        //             (int) (1.1 * (float) num_elements));

        // printf("-----------------PARALLEL READS--------------------\n");
        // // equilibrated
        // batch_parallel_reads(lsm_backup, num_reads, 0, num_elements);
        
        // // Skewed (at the end)
        // batch_parallel_reads(lsm_backup, num_reads, (int) (0.8 * (float) num_elements), num_elements);

        // // Skewed (with half outside)
        // batch_parallel_reads(lsm_backup, num_reads, (int) (0.9 * (float) num_elements),
        //             (int) (1.1 * (float) num_elements));

        // saving back LSTM
        print_state(lsm_backup);
        write_lsm_to_disk(lsm_backup);
        free_lsm(lsm_backup);
    }

}