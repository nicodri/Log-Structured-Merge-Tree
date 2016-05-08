#include "LSMTree.h"
#include <time.h>
#include <math.h>

#define FILENAME_SIZE 16

int main(){
    // LSMT parameters
    int Nc = 7;
    int num_elements = 1000000;
    int value_size = 32;

    // ------------------------ LSM Tree STRUCTURE

    int SIZE = 1000;
    int Cs_size[] = {SIZE, 3*SIZE, 9*SIZE, 27*SIZE, 81*SIZE, 3*81*SIZE, 729*SIZE, 3*729*SIZE, 1000000000};
    char name[] = "test";

    LSMTree_generation(name, Nc, Cs_size, value_size, num_elements, 0);


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
    read_test(lsm_backup, 1000001);
    read_test(lsm_backup, 1000002);
    read_test(lsm_backup, 1000003);

    // PARALLEL READING TEST
    printf("----------------------------------\n");
    printf("PARALLEL READS\n");
    printf("----------------------------------\n");
    read_parallel_test(lsm_backup, 999);
    printf("----------------------------------\n");
    read_parallel_test(lsm_backup, 9999);
    printf("----------------------------------\n");
    read_parallel_test(lsm_backup, 51215);
    printf("----------------------------------\n");
    read_parallel_test(lsm_backup, 215841);
    printf("----------------------------------\n");

    //update/delete tests
    // To store the value to update
    char* value = (char*) malloc(lsm_backup->value_size * sizeof(char));
    int key = 999;
    // Filling the updated value
    for (int i=0; i<lsm_backup->value_size - 7; i++) value[i] = 'b';
    sprintf(value + lsm_backup->value_size - 7, "_%d", key%10000);
    // update_lsm(lsm_backup, key, value);
    delete_lsm(lsm_backup, 999);
    read_test(lsm_backup, key);


    // Wrap up
    print_state(lsm_backup);
    write_lsm_to_disk(lsm_backup);
    free_lsm(lsm_backup);
}