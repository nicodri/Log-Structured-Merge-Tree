#include "LSMtree.h"

// Graph to plot (with and without Bloom)
//     - Generation time 
//     - batch reads (with different bloom parameters)
//             - all inside
//             - half inside, half outside
//     - batch updates

int main(){
    // LSMT parameters
    int Nc = 8;
    int value_size = 32;

    // ------------------------ LSM Tree STRUCTURE
    int SIZE = 1000;
    int Cs_size[] = {SIZE, 3*SIZE, 9*SIZE, 27*SIZE, 81*SIZE, 3*81*SIZE, 729*SIZE, 3*729*SIZE, 3*2187*SIZE, 3*6561*SIZE};
    char name[] = "test";

    // ------------ GENERATION TIME
    int num_elements_table []= {10000, 100000, 1000000, 10000000};
    int num_elements;
    int num_config = 4;

    double * generation_time = (double *) malloc(num_config * sizeof(double));

    for (int i=0; i<num_config; i++){
        num_elements = num_elements_table[i];
        generation_time[i] = LSMTree_generation(name, Nc, Cs_size, value_size, num_elements, 0);
    }

    printf("Generation time: \n");
    print_array_double(generation_time, num_config);

    // 
    // ------------ Batch Reads
    // Populating an LSM with 1 000 000  elmements
    num_elements = 1000000;
    LSMTree_generation(name, Nc, Cs_size, value_size, 1000000, 0);
    printf("Reading LSM from disk:\n");
    LSM_tree *lsm_backup = (LSM_tree *)malloc(sizeof(LSM_tree));
    read_lsm_from_disk(lsm_backup, name, FILENAME_SIZE);
    print_state(lsm_backup);

    int num_reads_table []= {1000, 10000, 50000, 100000};
    int num_reads;
    num_config = 4;

    double * reads_time_inside = (double *) malloc(num_config * sizeof(double));
    double * reads_time_half = (double *) malloc(num_config * sizeof(double));

    for (int i=0; i<num_config; i++){
        num_reads = num_reads_table[i];
        reads_time_inside[i] = batch_reads(lsm_backup, num_reads, 0, num_elements);
        reads_time_half[i] = batch_reads(lsm_backup, num_reads, (int) (0.8 * (float) num_elements),
                                         (int) (1.2 * (float) num_elements));
    }

    printf("Read time: \n");
    printf("Full inside\n");
    print_array_double(reads_time_inside, num_config);
    printf("Half inside\n");
    print_array_double(reads_time_half, num_config);

    // ---------- Batch updates
    int num_updates_table []= {10000, 100000, 500000, 1000000, 5000000};
    int num_updates;
    num_config = 5;

    double * updates_time = (double *) malloc(num_config * sizeof(double));

    for (int i=0; i<num_config; i++){
        num_updates = num_updates_table[i];
        updates_time[i] = batch_updates(lsm_backup, num_updates, (int) (0.4 * (float) num_elements),
                                        (int) (0.6 * (float) num_elements));
    }

    printf("Updates time: \n");
    printf("Full inside\n");
    print_array_double(updates_time, num_config);

    // Wrap up
    print_state(lsm_backup);
    write_lsm_to_disk(lsm_backup);
    free_lsm(lsm_backup);
}
