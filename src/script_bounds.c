#include "LSMTree.h"

// Report improvement with bounds check on reads
// Plots to display (throughput will be displayed)
//     - reads (uniform, skewed beginning, skewed middle, skewed end) * 2 (sorted/unsorted) * 2 (with/without bounds check)

int main(){
    // LSMT parameters
    int Nc = 7;
    int num_elements = 1000000;
    int value_size = 32;
    LSM_tree *lsm_backup;

    // ------------------------ LSM Tree STRUCTURE
    int SIZE = 1000;
    int Cs_size[] = {SIZE, 3*SIZE, 9*SIZE, 27*SIZE, 81*SIZE, 3*81*SIZE, 729*SIZE, 3*729*SIZE, 1000000000};
    char name[] = "test";

    LSMTree_generation(name, Nc, Cs_size, value_size, num_elements, 0);

    // reading from disk
    lsm_backup = (LSM_tree *)malloc(sizeof(LSM_tree));
    read_lsm_from_disk(lsm_backup, name, FILENAME_SIZE);
    print_state(lsm_backup);

    int num_reads_table []= {1000, 10000, 50000, 100000};
    int num_reads;
    int num_config = 4;
    
    double * read_uniform_time = (double *) malloc(num_config * sizeof(double));
    double * read_skewed_beginning_time = (double *) malloc(num_config * sizeof(double));
    double * read_skewed_middle_time = (double *) malloc(num_config * sizeof(double));
    double * read_skewed_end_time = (double *) malloc(num_config * sizeof(double));

    for (int i=0; i<num_config; i++){
        num_reads = num_reads_table[i];
        read_uniform_time[i] = batch_reads(lsm_backup, num_reads, 0, num_elements);
        read_skewed_beginning_time[i] = batch_reads(lsm_backup, num_reads, 0,
                                         (int) (0.2 * (float) num_elements));
        read_skewed_middle_time[i] = batch_reads(lsm_backup, num_reads, (int) (0.4 * (float) num_elements),
                                         (int) (0.6 * (float) num_elements));
        read_skewed_end_time[i] = batch_reads(lsm_backup, num_reads, (int) (0.8 * (float) num_elements),
                                         num_elements);
    }
    printf("read uniform time\n");
    print_array_double(read_uniform_time, num_config);
    printf("read skewed beginning time\n");
    print_array_double(read_skewed_beginning_time, num_config);
    printf("read skewed middle time\n");
    print_array_double(read_skewed_middle_time, num_config);
    printf("read skewed end time\n");
    print_array_double(read_skewed_end_time, num_config);

    free_lsm(lsm_backup);
}