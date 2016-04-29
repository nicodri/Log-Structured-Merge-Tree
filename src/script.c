#include "LSMTree.h"
#include <time.h>

// hard coding of value to test
#define SIZE 1000
#define VALUE_SIZE 10
#define FILENAME_SIZE 16

// Read a batch of random keys from an LSM_tree inside the range [key_down, key_up]
void batch_random_read(LSM_tree *lsm, int num_reads, int key_down, int key_up){
    // To store the value_read
    char* value_read = (char*) malloc(lsm->value_size*sizeof(char));
    int index, check;
    // Initialize the seed
    srand(time(NULL));
    for (int i=0; i<num_reads; i++){
        index = (int)(key_down + (rand()/(float)RAND_MAX) * (key_up - key_down));
        check = read_lsm(lsm, index, value_read);
        if (check == 1){
            if (VERBOSE == 1) printf("Reading key: %d; value found: %s\n",
                                       index, value_read);
        }
        else {
            if (VERBOSE == 1) printf("Reading key: %d; key not found:\n", index);
        }
        free(value_read);
    } 
}

// Update the batch of keys in the range [key_down, key_up]
void batch_updates(LSM_tree *lsm, int key_down, int key_up){
    char value[VALUE_SIZE];
    for (int i=key_down; i < key_up; i++){
        // Filling value
        sprintf(value, "update%d", i%150);
        update_lsm(lsm, i, value);
    }
}
// delete the batch of keys in the range [key_down, key_up]
void batch_deletes(LSM_tree *lsm, int key_down, int key_up){
    for (int i=key_down; i < key_up; i++){
        delete_lsm(lsm, i);
    }
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
}

void batch_read_disk_component(LSM_tree *lsm, int disk_index, int num_keys){
    char * comp_id = (char *) malloc(10*sizeof(char));
    sprintf(comp_id, "C%d", disk_index);

    component * C = (component *) malloc(sizeof(component));
    read_disk_component(C, lsm->name, lsm->Cs_Ne + disk_index, comp_id,
                        lsm->Cs_size + disk_index, VALUE_SIZE, FILENAME_SIZE);

    // Printing
    printf("Number of elements in component %s: %d\n", comp_id, *(lsm->Cs_Ne + disk_index));
    printf("First %d keys \n", num_keys);
    for (int i=0; i < num_keys; i++){
        printf("%d \n", C->keys[i]);
    }

    // Free memory
    free(comp_id);
    free(C);
}

// Generate and save to disk an LSM-Tree
void lsm_generation(char* name, int size_test){
    // Creating lsm structure:
    int Nc = 6;
    // List contains Nc+2 elements: [C0, buffer, C1, C2,...]
    // Last component with very big size (as finite number of components chosen)
    int Cs_size[] = {SIZE, 3*SIZE, 9*SIZE, 27*SIZE, 81*SIZE, 729*SIZE, 9*729*SIZE, 1000000000};

    LSM_tree *lsm = (LSM_tree*) malloc(sizeof(LSM_tree));
    build_lsm(lsm, name, Nc, Cs_size, VALUE_SIZE, FILENAME_SIZE);

    // ---------------- TEST OF APPENDING
    // filling the lsm
    char value[VALUE_SIZE];

    // Sorted keys
    // for (int i=0; i < size_test; i++){
    //     // Filling value
    //     sprintf(value, "hello%d", i%150);
    //     insert_lsm(lsm, i, value);
    // }    

    // Unsorted keys
    // adding even keys
    int j;
    for (int i=0; i < size_test/2; i++){
        j = size_test/2 - i - 1;
        // Filling value
        sprintf(value, "hello%d", (2*j)%150);
        insert_lsm(lsm, (2*j), value);
    }
    // adding odd keys
    for (int i=size_test/2; i < size_test; i++){
        // Filling value
        sprintf(value, "hello%d", (2*(i-size_test/2) + 1)%150);
        insert_lsm(lsm, 2*(i-size_test/2) + 1, value);
    }

    // Print status of the lsm tree
    print_state(lsm);

    // Print some sequence of keys
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

    // ---------------- TEST READING COMPONENT FROM DISK
    // batch_read_disk_component(lsm, 0, 10);

    // // STOP
    // return

    // Writing to disk
    write_lsm_to_disk(lsm);

    // Free memory
    free_lsm(lsm);
}

//Test storing on disk an array
int main(){
    char name[] = "test";
    int size_test = 100000;

    // ---------------- TEST GENERATING LSM
    // lsm_generation(name, size_test);

    // ---------------- TEST READING LSM FROM DISK
    printf("Reading LSM from disk:\n");
    LSM_tree *lsm_backup = (LSM_tree *)malloc(sizeof(LSM_tree));
    read_lsm_from_disk(lsm_backup, name, FILENAME_SIZE);
    print_state(lsm_backup);

    // ---------------- TEST READING
    // batch_random_read(lsm_backup, 5, 0, 2*size_test);
    
    // ---------------- TEST UPDATING
    // printf("BEFORE UPDATE\n");
    // read_test(lsm_backup, size_test/2);
    // read_test(lsm_backup, size_test/2 + 1000 - 1);
    // batch_updates(lsm_backup, size_test/2, size_test/2 + 1000);
    // printf("AFTER UPDATE\n");
    // read_test(lsm_backup, size_test/2);
    // read_test(lsm_backup, size_test/2 + 1000 - 1);
    // print_state(lsm_backup);

    // // ---------------- TEST DELETING
    // printf("BEFORE DELETE\n");
    // read_test(lsm_backup, size_test/2);
    // read_test(lsm_backup, size_test/2 + 1000 - 1);
    // batch_deletes(lsm_backup, size_test/2, size_test/2 + 1000);
    // printf("AFTER DELETE\n");
    // read_test(lsm_backup, size_test/2);
    // read_test(lsm_backup, size_test/2 + 1000 - 1);
    // print_state(lsm_backup);

    // PARALLEL READING TEST
    read_test(lsm_backup, 999);
    read_test(lsm_backup, 9999);
    read_test(lsm_backup, 998000);
    printf("----------------------------------\n");
    printf("PARALLEL READS\n");
    printf("----------------------------------\n");
    read_parallel_test(lsm_backup, 999);
    printf("----------------------------------\n");
    read_parallel_test(lsm_backup, 9999);
    printf("----------------------------------\n");
    read_parallel_test(lsm_backup, 998000);

    // Free memory
    free_lsm(lsm_backup);

    printf("Work done\n");
}