#include "LSMTree.h"

// Allocate and set filename to 'name/component_typecomponent_id.data'
void get_files_name(char *filename, char *name, char* component_id, char* component_type,
                     int filename_size){
    // Check if memory was allocated
    if (filename == NULL) filename = (char *) calloc(filename_size + 8,sizeof(char));
    sprintf(filename, "%s/%c%s.data", name, *component_type, component_id);
}

// Same signature than get_files_name but only for disk component
// i.e. component_index is an int and component_id will be set to "Ccomponent_index"
void get_files_name_disk(char *filename, char *name, int component_index,
                         char* component_type, int filename_size){
    // Check if memory was allocated
    if (filename == NULL) filename = (char *) calloc(filename_size + 8,sizeof(char));
    sprintf(filename, "%s/%cC%d.data", name, *component_type, component_index);
}

// Binary search of key inside sorted integer array keys[down,..,top]
// return global index in keys if found else -1
int binary_search(int* keys, int key, int down, int top){
    if (top < down) return -1;
    int middle = (top + down)/2;
    if (keys[middle] < key) return binary_search(keys, key, middle+1, top);
    if (keys[middle] > key) return binary_search(keys, key, down, middle-1);
    return middle;
}

int binary_search_signal(int* keys, int key, int down, int top,
                         int thread_level, int* shared_level, int next_check){
    // printf("Thread %d next check %d\n", thread_level, next_check);
    if (next_check == 0){
        // printf("Thread %d check counter set to %d\n", thread_level, *shared_level);
        // We dont use the semaphore to read
        if (thread_level > *shared_level) return -1;
        // reset the counter;
        else next_check = FREQUENCE;
    }
    else next_check -= 1;
    if (top < down) return -1;
    int middle = (top + down)/2;
    if (keys[middle] < key) return binary_search_signal(keys, key, middle+1, top,
                                                        thread_level, shared_level,
                                                        next_check);
    if (keys[middle] > key) return binary_search_signal(keys, key, down, middle-1,
                                                        thread_level, shared_level,
                                                        next_check);
    return middle;
}

// Linear search of key in (unsorted) keys. Set index to the position if found
void keys_linear_search(int* index, int key, int* keys, int Ne){
    *index = -1;
    for (int i=0; i < Ne; i++){
        if (keys[i] == key){
            *index = i;
        }
    }
}

void merge_with_values(int* keys, char* values, int down, int middle, int top,
                       int value_size){
    int size = top - down + 1;

    // Copying list and values (indices shifted of down)
    int* temp_keys = (int*) malloc(size * sizeof(int));
    char* temp_values = (char*) malloc(value_size * size * sizeof(char));
    for (int i=0; i < size; i++){
        temp_keys[i] = keys[i + down];
        strcpy(temp_values + i*value_size, values + (i+down)*value_size);
    }

    // Going through the sublists
    int ileft = 0;
    int iright = middle + 1 - down;
    int i = down;
    while ((ileft + down <= middle) && (iright + down <= top)){
        if (temp_keys[ileft] > temp_keys[iright]){
            keys[i] = temp_keys[iright];
            strcpy(values + (i++)*value_size, temp_values + (iright++)*value_size);
        }
        else{
            keys[i] = temp_keys[ileft];
            strcpy(values + (i++)*value_size, temp_values + (ileft++)*value_size);
        }
    }

    // Finishing the filling
    while (ileft + down <= middle){
        keys[i] = temp_keys[ileft];
        strcpy(values + (i++)*value_size, temp_values + (ileft++)*value_size);
    }
    while (iright + down <= top){
        keys[i] = temp_keys[iright];
        strcpy(values + (i++)*value_size, temp_values + (iright++)*value_size);
    }

    // Freeing memory
    free(temp_keys);
    free(temp_values);

}

// Merge two sorted lists of keys and update the corresponding values from the two
// values list in the array of values; the results are set in the second list of
// keys and values (corresponds to the next component)
// Tested: ok
void merge_list(int* keys1, int* keys2, char* values1, char* values2,
                      int* Ne1, int* Ne2, int value_size){
    // Temporary files with a copy of keys2 and values2 because
    // both files are modified inplace
    int* keys2_temp = (int *) malloc((*Ne2) * sizeof(int));
    for (int i=0; i<(*Ne2); i++) keys2_temp[i] = keys2[i];
    char* values2_temp = (char *) malloc(value_size * (*Ne2) * sizeof(char));
    for (int i=0; i<(*Ne2); i++) strcpy(values2_temp + i*value_size,
                                       values2 + i*value_size);

    // Going through the sublists
    int ileft = 0;
    int iright = 0;
    int i = 0;
    int number_merges=0; // Count updates/deletes to update metadata
    while ((ileft < (*Ne1)) && (iright < (*Ne2))){
        // if (i >= *Ne2) printf("ERROR: i is %d, Ne1: %d, Ne2: %d, iright: %d , ileft: %d\n", i,
        //                       *Ne1, *Ne2, iright, ileft);
        // Filling from keys2_temp
        if (keys1[ileft] > keys2_temp[iright]){
            keys2[i] = keys2_temp[iright];
            strcpy(values2 + (i++)*value_size,
                   values2_temp + (iright++)*value_size);
        }
        // Filling from keys1
        else if (keys1[ileft] < keys2_temp[iright]){
            keys2[i] = keys1[ileft];
            strcpy(values2 + (i++)*value_size, values1 + (ileft++)*value_size);
        }
        // Case with equality (when updates/delete operation)
        else{
            // Same behavior for updates/deletes, we keep the most recent one (upper)
            // printf("DEBUG: Update/delete case\n");
            // printf("On key: %d\n", keys1[ileft]);
            // printf("Value left: %s \n", values1 + (ileft)*value_size);
            // printf("Right key: %d\n", keys2[iright]);
            // printf("Value right: %s \n", values2_temp + (iright)*value_size);
            keys2[i] = keys1[ileft];
            strcpy(values2 + (i++)*value_size,
                   values1 + (ileft++)*value_size);
            iright++;
            number_merges++;
        }
    }
    // Update number of elements in component (because of updates/deletes)
    *Ne2 = *Ne2 - number_merges;

    // Finishing to fill
    while (ileft < (*Ne1)){
        keys2[i] = keys1[ileft];
        strcpy(values2 + (i++)*value_size, values1 + (ileft++)*value_size);
    }
    while (iright < (*Ne2)){
        keys2[i] = keys2_temp[iright];
        strcpy(values2 + (i++)*value_size, values2_temp + (iright++)*value_size);
    }
    // Freeing the pointers
    free(keys2_temp);
    free(values2_temp);
}

// Sort inplace keys and values accordingly
// down: first index
// top: last index (i.e. size - 1)
// Tested: ok
void merge_sort_with_values(int* keys, char* values, int down, int top, int value_size){
    if (top - down > 0) {
        int middle = (top + down) / 2;
        // Sorting left
        merge_sort_with_values(keys, values, down, middle, value_size);
        // Sorting right
        merge_sort_with_values(keys, values, middle + 1, top, value_size);
        // Merging
        merge_with_values(keys, values, down, middle, top, value_size);
    }
}