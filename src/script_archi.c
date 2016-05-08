#include "LSMTree.h"

// Basic behavior of the LSM tree for different architecture
// Plots to display (throughput will be displayed)
//     - generation
//     - reads (uniform, skewed beginning, skewed end) * 2 (sorted/unsorted)
//     - updates (uniform, skewed beginning, skewed end) * 2 (sorted/unsorted)

int main(){
    // LSMT parameters
    int Nc = 7;
    int num_elements = 1000000;
    int num_reads = 10000;
    int num_updates = 100000;
    int value_size = 32;

    // ------------------------ LSM Tree STRUCTURE
    int* Cs_size = (int *) malloc((Nc+2) * sizeof(int));
    char name[] = "test";

    int size_0[] = {500, 1000};
    int sizes = 2;
    int ratio[] = {3, 5, 7};
    int ratios = 3;

    int num_config = sizes*ratios;

    double * generation_time = (double *) malloc(num_config * sizeof(double));
    
    double * read_uniform_time = (double *) malloc(num_config * sizeof(double));
    double * read_skewed_beginning_time = (double *) malloc(num_config * sizeof(double));
    double * read_skewed_end_time = (double *) malloc(num_config * sizeof(double));
    
    double * update_uniform_time = (double *) malloc(num_config * sizeof(double));
    double * update_skewed_beginning_time = (double *) malloc(num_config * sizeof(double));
    double * update_skewed_end_time = (double *) malloc(num_config * sizeof(double));

    int config = 0;
    LSM_tree *lsm;
    for (int s=0; s<sizes; s++){
        Cs_size[0] = size_0[s];
        for (int r=0; r<ratios; r++){
            printf("config = 'size %d ratio %d \n", Cs_size[0], ratio[r]);
            // Building the sizes
            for (int i=1; i<(Nc+2); i++) Cs_size[i] = ratio[r] * Cs_size[i-1];
            // generation
            lsm = (LSM_tree*) malloc(sizeof(LSM_tree));
            generation_time[config] = LSMTree_generation(name, Nc, Cs_size, value_size, num_elements, 0, lsm);

            // reading lsmt from disk
            // LSM_tree *lsm_backup = (LSM_tree *)malloc(sizeof(LSM_tree));
            // read_lsm_from_disk(lsm_backup, name, FILENAME_SIZE);
            // print_state(lsm_backup);

            // reading
            // equilibrated
            read_uniform_time[config] = batch_reads(lsm, num_reads, 0, num_elements);
            
            // Skewed (at the beginning)
            read_skewed_beginning_time[config] = batch_reads(lsm, num_reads, 0, (int) (0.2 * (float) num_elements));

            // Skewed (at the end)
            read_skewed_end_time[config] = batch_reads(lsm, num_reads, (int) (0.8 * (float) num_elements), num_elements);

            // updating
            // equilibrated
            update_uniform_time[config] = batch_updates(lsm, num_updates, 0, num_elements);
            
            free_lsm(lsm);
            lsm = (LSM_tree*)malloc(sizeof(LSM_tree));
            generation_time[config] = LSMTree_generation(name, Nc, Cs_size, value_size, num_elements, 0, lsm);

            // Skewed (at the beginning)
            update_skewed_beginning_time[config] = batch_updates(lsm, num_updates, 0, (int) (0.2 * (float) num_elements));

            free_lsm(lsm);
            lsm = (LSM_tree *)malloc(sizeof(LSM_tree));
            LSMTree_generation(name, Nc, Cs_size, value_size, num_elements, 0, lsm);

            // Skewed (at the end)
            update_skewed_end_time[config] = batch_updates(lsm, num_updates, (int) (0.8 * (float) num_elements), num_elements);

            config++;
            free_lsm(lsm);

        }
    }
    printf("generation time\n");
    print_array_double(generation_time, num_config);
    printf("read uniform time\n");
    print_array_double(read_uniform_time, num_config);
    printf("read skewed beginning time\n");
    print_array_double(read_skewed_beginning_time, num_config);
    printf("read skewed end time\n");
    print_array_double(read_skewed_end_time, num_config);
    printf("udpate time\n");
    print_array_double(update_uniform_time, num_config);
    printf("update skewed beginning time\n");
    print_array_double(update_skewed_beginning_time, num_config);
    printf("update skewed end time\n");
    print_array_double(update_skewed_end_time, num_config);

}