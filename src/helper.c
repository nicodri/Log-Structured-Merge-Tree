#include "LSMTree.h"

// Allocate and set filename to 'name/component_typecomponent_id.data'
char* get_files_name(char *name, char* component_id, char* component_type,
                     int filename_size){
    char *filename = (char *) calloc(filename_size + 8,sizeof(char));
    sprintf(filename, "%s/%s%s.data", name, component_type, component_id);

    return filename;
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
void merge_components(int* keys1, int* keys2, char* values1, char* values2,
                      int* size1, int* size2, int value_size){
    // Temporary files with a copy of keys2 and values2 because
    // both files are modified inplace
    int* keys2_temp = (int *) malloc((*size2) * sizeof(int));
    for (int i=0; i<(*size2); i++) keys2_temp[i] = keys2[i];
    char* values2_temp = (char *) malloc(value_size * (*size2) * sizeof(char));
    for (int i=0; i<(*size2); i++) strcpy(values2_temp + i*value_size,
                                       values2 + i*value_size);

    // Going through the sublists
    int ileft = 0;
    int iright = 0;
    int i = 0;
    while ((ileft < (*size1)) && (iright < (*size2))){
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
            // Update case
            // TOFIX: list will contain holes at the end
            if (*(values1 + (ileft)*value_size) != '!'){
                printf("DEBUG: Update case\n");
                printf("On key: %d\n", keys1[ileft]);
                printf("Value left: %s \n", values1 + (ileft)*value_size);
                printf("Right key: %d\n", keys2[iright]);
                printf("Value right: %s \n", values2_temp + (iright)*value_size);
                keys2[i] = keys2_temp[ileft];
                strcpy(values2 + (i++)*value_size,
                       values1 + (ileft++)*value_size);
                iright++;
                // Update number of elements
                *size1 = *size1 - 1;
            }
            // Delete case
            else{
                printf("DEBUG: Delete case\n");
                ileft++;
                iright++;
                // Update number of elements
                *size1 = *size1 - 1;
                *size2 = *size2 - 1;
            }
        }
    }
    // Finishing the filling
    while (ileft < (*size1)){
        keys2[i] = keys1[ileft];
        strcpy(values2 + (i++)*value_size, values1 + (ileft++)*value_size);
    }
    while (iright < (*size2)){
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

// Testing merge_sort
// int main(){
//     int value_size = 16;
//     // Keys
//     int size = 1000;
//     int *keys = (int *) malloc(size * sizeof(int));
//     for (int i=0; i<size/2; i++) keys[i] = 2*i;
//     for (int i=size/2; i<size; i++) keys[i] = 2*(i-size/2) + 1;

//     // Values
//     char* values = (char *) malloc(size*value_size*sizeof(char));
//     char* value = (char *) malloc(value_size);
//     for (int i=0; i<size/2; i++){
//         // Filling value
//         sprintf(value, "hello%d", (2*i)%150);
//         strcpy(values + i*value_size, value);
//     }
//     for (int i=size/2; i<size; i++){
//         // Filling value
//         sprintf(value, "hello%d", (2*(i-size/2) + 1)%150);
//         strcpy(values + i*value_size, value);
//     }
//     printf("Keys before Merge\n");
//     for (int i = 0; i<size; i++) printf("%d\n", keys[i]);

//     printf("Values before Merge\n");
//     for (int i = 0; i<size; i++) printf("%s\n", values + i*value_size);

//     merge_sort_with_values(keys, values, 0, size-1, value_size);

//     printf("Keys after Merge\n");
//     for (int i = 0; i<size; i++) printf("%d\n", keys[i]);

//     printf("Values after Merge\n");
//     for (int i = 0; i<size; i++) printf("%s\n", values + i*value_size);


// }

// Testing merge_component
// int main(){
//     int value_size = 16;
//     // Keys1
//     int size1 = 1000;
//     int *keys1 = (int *) malloc(size1 * sizeof(int));
//     for (int i=0; i<size1; i++) keys1[i] = 2*i;

//     int size2 = 1000;
//     int *keys2 = (int *) malloc((size2 + size1) * sizeof(int));
//     for (int i=0; i<size2; i++) keys2[i] = 2*i + 1;

//     // Values1
//     char* values1 = (char *) malloc(size1*value_size*sizeof(char));
//     char* value = (char *) malloc(value_size);
//     for (int i=0; i < size1; i++){
//         // Filling value
//         sprintf(value, "hello%d", (2*i)%150);
//         strcpy(values1 + i*value_size, value);
//     }

//     // Values2
//     char* values2 = (char *) malloc((size1+size2)*value_size*sizeof(char));
//     for (int i=0; i < size1; i++){
//         // Filling value
//         sprintf(value, "hello%d", (2*i+1)%150);
//         strcpy(values2 + i*value_size, value);
//     }

//     printf("Keys before Merge\n");
//     for (int i = 0; i<10; i++) printf("%d\n", keys2[i]);

//     printf("Values before Merge\n");
//     for (int i = 0; i<10; i++) printf("%s\n", values2 + i*value_size);

//     merge_components(keys1, keys2, values1, values2, size1, size2, value_size);

//     printf("Last Keys after Merge\n");
//     for (int i = (size1 + size2)-10; i<(size1 + size2); i++) printf("%d\n", keys2[i]);

//     printf("Last Values after Merge\n");
//     for (int i = (size1 + size2)-10; i<(size1 + size2); i++) printf("%s\n", values2 + i*value_size);
// }