#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <signal.h>
#include <limits.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>

// verbose to debugg (1 activated, else 0)
#define VERBOSE 1
// Indicator for a deleted key
#define TOMBSTONE ((const char *)"!")

// ********************************************************
// Parameters that may be changed by the user:
#define FILENAME_SIZE 16
// Tolerance of loss in C0
#define CO_TOLERANCE 2000
// Frequency of check in parallel read
#define FREQUENCE 20
// Define if use of a bloom filter
#define BLOOM_ON 1
#define HASHES 5
#define BLOOM_SIZE 10000000
// ********************************************************

// Global semaphore for parallel read
sem_t* mutex;

// Bloom Filter struct
typedef uint64_t index_t;
typedef uint64_t key_t_;
typedef struct {
    int hashes;
    index_t size; // in bits
    index_t count; // in bits
    index_t *table;
} bloom_filter_t;

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
    bloom_filter_t *bloom;
} LSM_tree;

// Struct used for the parallel implementation
typedef struct arg_thread_common
{
    int key;
    int filename_size;
    int* shared_level;
    char* name;
} arg_thread_common;

typedef struct arg_thread
{
    int thread_id;
    int Cs_Ne;
    arg_thread_common* common;
} arg_thread;


// A min heap node
typedef struct MinHeapNode
{
    int element; // The element to be stored
    int i; // index of the array from which the element is taken
    int j; // index of the next element to be picked from array
} MinHeapNode;

typedef struct MinHeap
{
    MinHeapNode *harr; // pointer to array of elements in heap
    int heap_size; // size of min heap    
}MinHeap;


// Declarations for LSMTree.c
void init_lsm(LSM_tree *lsm, char* name, int filename_size);
void create_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size, int filename_size);
void free_lsm(LSM_tree *lsm);
void build_lsm(LSM_tree *lsm, char* name, int Nc, int* Cs_size, int value_size,
               int filename_size);
void write_lsm_to_disk(LSM_tree *lsm);
void read_lsm_from_disk(LSM_tree *lsm, char *name, int filename_size);
void append_lsm(LSM_tree *lsm, int key, char *value);
void insert_lsm(LSM_tree *lsm, int key, char *value);
int read_lsm(LSM_tree *lsm, int key, char* value);
int read_lsm_parallel(LSM_tree *lsm, int key, char* value);
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
void read_value(char* value, int index, char* name, int component_index, int value_size,
                int filename_size);
void swap_component_pointer(component *current_component, component *next_component,
                            int value_size);
void merge_components(component* next_component, component* current_component, char* name,
                      int value_size, int filename_size);
void component_search(int* index, int key, int length, char* filename);
void *component_search_parallel(void *argument);

// Declarations for helper.c
void get_files_name(char *filename, char *name, char* component_id, char* component_type,
                     int filename_size);
void get_files_name_disk(char *filename, char *name, int component_index,
                         char* component_type, int filename_size);
int binary_search(int* keys, int key, int down, int top);
int binary_search_signal(int* keys, int key, int down, int top,
                         int thread_level, int* shared_level, int next_check);
void keys_linear_search(int* index, int key, int* keys, int Ne);                     
void merge_with_values(int* keys, char* values, int down, int middle, int top,
                       int value_size);
void merge_list(int* keys1, int* keys2, char* values1, char* values2,
                int* size1, int* size2, int value_size);
void merge_sort_with_values(int* keys, char* values, int down, int top, int value_size);

// Declarations for bloom.c
void set_bit(bloom_filter_t *B, index_t i);
index_t get_bit(bloom_filter_t *B, index_t i);
index_t hash1(bloom_filter_t *B, key_t_ k);
index_t hash2(bloom_filter_t *B, key_t_ k);
void bloom_init(bloom_filter_t *B, index_t size_in_bits, int hashes);
void bloom_destroy(bloom_filter_t *B);
int bloom_check(bloom_filter_t *B, key_t_ k);
void bloom_add(bloom_filter_t *B, key_t_ k);

// Declarations for heap.c
void MinHeap_init(MinHeap *minheap, MinHeapNode* a, int size);
void MinHeapify(MinHeap * minheap, int i);
MinHeapNode getMin(MinHeap *minheap);
void replaceMin(MinHeap *minheap, MinHeapNode x);
void swap(MinHeapNode *x, MinHeapNode *y);
void mergeKArrays(int *output, int *arr, int k, int n);

// Declarations for exp.c
void print_array_int(int* array, int size);
void print_array_double(double* array, int size);
void read_test(LSM_tree* lsm, int key);
void read_parallel_test(LSM_tree* lsm, int key);
double LSMTree_generation(char*name, int Nc, int* Cs_size, int value_size, int num_elements,
                        int sorted);
double batch_updates(LSM_tree *lsm, int num_updates, int key_down, int key_up);
double batch_reads(LSM_tree *lsm, int num_reads, int key_down, int key_up);
double batch_parallel_reads(LSM_tree *lsm, int num_reads, int key_down, int key_up);
