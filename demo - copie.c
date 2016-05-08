#include "LSMTree.h"

int main(){
    
    
    printf("You entered: %d\n", a);

    // LSMT parameters from user inputs
    int Nc, size, ratio, value_size;
    printf("Define the number of components: ");
    scanf("%d", &Nc);
    printf("Define the size of C0: ");
    scanf("%d", &size);
    printf("Define the ratio: ");
    scanf("%d", &ratio);
    printf("Define the value size: ");
    scanf("%d", &value_size);
    char *name = (char *) malloc(FILENAME_SIZE * sizeof(char));
    printf("Define the name of the LSM tree: ");
    scanf("%s", name);

    // ------------------------ BUILDING THE LSM TREE
    int* Cs_size = (int *) malloc((Nc+2) * sizeof(int));
    Cs_size[0] = size;
    for (int i=1; i<(Nc+2); i++) Cs_size[i] = ratio * Cs_size[i-1];

    LSM_tree  *lsm =  (LSM_tree*) malloc(sizeof(LSM_tree));
    

    // Read test
    read_test(lsm, 100);
    read_test(lsm, 1000);
    read_test(lsm, 10000);
    read_test(lsm, 100000);
    read_test(lsm, 999999);
    read_test(lsm, 1000000);


    // ---------------- TEST READING LSM FROM DISK
    printf("%d\n",BLOOM_ON);
    printf("%d\n",BLOOM_ON);
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