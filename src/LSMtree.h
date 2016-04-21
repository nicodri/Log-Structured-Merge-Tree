#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

// verbose of some functions (1 activated, else 0)
#define VERBOSE 1
#define TOMBSTONE ((const char *)"!")
// Tolerance of loss in C0
#define CO_TOLERANCE 1100

typedef struct component {
    int *keys;
    char *values;
    int *Ne; // number of elements stored (point to the int inside the list of the LSMtree)
    int *S; // capacity (point to the int inside the list of the LSMtree)
    char* component_id; //identifier of the component for the filename (Cnumber or buffer)
} component;

// First version: finit number of components
typedef struct LSM_tree {
    char *name;
    // C0 and buffer are in main memory
    component *C0;
    component *buffer;
    int Ne; // Total number of key/value tuples stored
    int Nc; // Number of file components, ie components on disk
    int value_size; // Upper bound on the value size (in number of chars)
    int filename_size; // Size of the name, will be used to mainpulate filename
    // TODO: linked list for infinite number oc components?
    int *Cs_Ne; // List of number of elements per component: [C0, buffer, C1, C2,...]
    int *Cs_size; // List of number of elements per component: [C0, buffer, C1, C2,...]
} LSM_tree;

// Declarations for LSMTree.c
void init_lsm(LSM_tree *lsm, char* name, int filename_size);
void create_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size, int filename_size);
void free_lsm(LSM_tree *lsm);
void build_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size,
               int filename_size);
void write_lsm_to_disk(LSM_tree *lsm);
void read_lsm_from_disk(LSM_tree *lsm, char *name, int filename_size);
void append_lsm(LSM_tree *lsm, int key, char *value);
char* read_lsm(LSM_tree *lsm, int key);
void update_lsm(LSM_tree *lsm, int key, char *value);
void delete_lsm(LSM_tree *lsm, int key);
void update_component_size(LSM_tree *lsm);
void print_state(LSM_tree *lsm);

// Declarations for component.c
void init_component(component * c, int* component_size, int value_size, int* Ne,
                    char* component_id);
void free_component(component *c);
void create_disk_component(char* name, int Nc, int filename_size);
void read_disk_component(component* C, char *name, int* Ne, char *component_id,
                         int* component_size, int value_size, int filename_size);
void write_disk_component(component *pC, char *name, int value_size, int filename_size);
void append_on_disk(component * C, int N, char* name, int value_size,
                    int filename_size);
void read_value(char* value, int index, char* name, char* component_id, int value_size,
                int filename_size);
void swap_component_pointer(component *current_component, component *next_component,
                            int value_size);
void merge_components(component* next_component, component* current_component, char* name,
                      int value_size, int filename_size);
void component_search(int* index, int key, int length, char* filename);

// Declarations for helper.c
void get_files_name(char *filename, char *name, char* component_id, char* component_type,
                     int filename_size);
int binary_search(int* keys, int key, int down, int top);
void keys_linear_search(int* index, int key, int* keys, int Ne);                     
void merge_with_values(int* keys, char* values, int down, int middle, int top,
                       int value_size);
void merge_list(int* keys1, int* keys2, char* values1, char* values2,
                int* size1, int* size2, int value_size);
void merge_sort_with_values(int* keys, char* values, int down, int top, int value_size);
