#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

void merge_with_positions(int list[], int positions[], int down, int middle, int top){
    int size = top - down + 1;

    // Copying list and positions
    int templist[size];
    int temp_positions[size];
    for (int i=0; i < size; i++){
        templist[i] = list[i + down];
        temp_positions[i] = positions[i + down];
    }

    // Going through the sublists
    int ileft = down;
    int iright = middle + 1;
    int i = down;
    while ((ileft <= middle) && (iright <= top)){
        if (templist[ileft - down] > templist[iright - down]){
            list[i] = templist[iright - down];
            positions[i++] = temp_positions[iright++ - down];
        }
        else{
            list[i] = templist[ileft - down];
            positions[i++] = temp_positions[ileft++ - down];
        }
    }

    // Finishing the filling
    while (ileft <= middle){
        list[i] = templist[ileft - down];
        positions[i++] = temp_positions[ileft++ - down];
    }
    while (iright <= top){
        list[i] = templist[iright - down];
        positions[i++] = temp_positions[iright++ - down];
    }

}

// Merge two sorted lists and updates the positions in the new list in
// the two positions list
void merge_components(int list1[], int list2[], int list[], int positions[],
                       int size1, int size2){
    // Going through the sublists
    int ileft = 0;
    int iright = 0;
    int i = 0;
    while ((ileft < size1) && (iright < size2)){
        // Filling from list2
        if (list1[ileft] > list2[iright]){
            list[i] = list2[iright];
            positions[size1 + iright++] = i++;
        }
        // Filling from list1
        else{
            list[i] = list1[ileft];
            positions[ileft++] = i++;
        }
    }

    // Finishing the filling
    while (ileft < size1){
        list[i] = list1[ileft];
        positions[ileft++] = i++;
    }
    while (iright < size2){
        list[i] = list2[iright];
        positions[size1 + iright++] = i++;
    }
}

// Sort list and argsort accordingly positions
void merge_sort_with_positions(int list[], int positions[], int down, int top){
    if (top - down > 0) {
        int middle = (top + down) / 2;
        // Sorting left
        merge_sort_with_positions(list, positions, down, middle);
        // Sorting right
        merge_sort_with_positions(list, positions, middle + 1, top);
        // Merging
        merge_with_positions(list, positions, down, middle, top);
    }
}

// Reorder list of values according to new position from positions
// positions: stores positions of values in sorted array at the index corresponding
//            to the old position (inside values)
void update_values(int* values, int* positions, int value_size, int values_length){
    // Output
    int* new_values = (int *) malloc(values_length*value_size*sizeof(char));

    for (int i=0; i<values_length; i++){
        for (int k=0; k < value_size; k++){
            new_values[i*value_size + k] = values[positions[i]*value_size + k];
        }
    }
    // Swap input and output
    values = new_values;
}

// Merge list of values according to new position from positions
// positions: stores the concatenation of the positions for values1 and values2
//            in the output values (merging values1 and valeus2) at each index
//            to the old position (inside values)
int* build_values(int* values1, int* values2, int* positions, int len1,
                  int len2, int value_size){
    // Output
    int output_length = len1 + len2;
    int* new_values = (int *) malloc(output_length*value_size*sizeof(char));

    for (int i=0; i<len1; i++){
        for (int k=0; k < value_size; k++){
            new_values[positions[i]*value_size + k] = values1[i*value_size + k];
        }
    }
    for (int i=len1; i<output_length; i++){
        for (int k=0; k < value_size; k++){
            new_values[positions[i]*value_size + k] = values2[(i - len1)*value_size + k];
        }
    }
    return new_values;
}

// int main(){
//     // testing swap
//     int list1[] = {1, 4, 5, 6};
//     int list2[] = {2, 3, 7, 8};
//     int *plist1 = list1;
//     int *plist2 = list2;
//     printf("list1\n");
//     for (int i=0; i<4; i++) printf("%i, ", plist1[i]);
//     printf("\n");
//     printf("list2\n");
//     for (int i=0; i<4; i++) printf("%i, ", plist2[i]);
//     printf("\n");
    
//     // Swap list pointer
//     swap(plist1, plist2);
//     // int *temp = plist2;
//     // plist2 = plist1;
//     // plist1 = temp;
//     printf("list1\n");
//     for (int i=0; i<4; i++) printf("%i, ", plist1[i]);
//     printf("\n");
//     printf("list2\n");
//     for (int i=0; i<4; i++) printf("%i, ", plist2[i]);
//     printf("\n");


// }

// int main(){
//     //Test code
//     int i;
//     int size=9;
//     int test[] = {0, 5, 2, 3, 1 , 8, 7, 6, 4};
//     int positions[size];
//     for (i=0; i<size; i++) positions[i] = i;
//     for (i=0; i<size; i++) printf("%i, ", test[i]);
//     printf("\n");
//     for (i=0; i<size; i++) printf("%i, ", positions[i]);
//     printf("\n");
//     merge_sort_with_positions(test, positions, 0, size-1);
//     for (i=0; i<size; i++) printf("%i, ", test[i]);
//     printf("\n");
//     for (i=0; i<size; i++) printf("%i, ", positions[i]);
//     printf("\n");

//     printf("Testing on 2 lists\n");
//     int size1 = 4;
//     int size2 = 4;
//     int new_size = size1 + size2;
//     int list1[] = {1, 4, 5, 6};
//     int list2[] = {2, 3, 7, 8};
//     for (i=0; i<size1; i++) printf("%i, ", list1[i]);
//     printf("\n");
//     for (i=0; i<size2; i++) printf("%i, ", list2[i]);
//     printf("\n");
//     int list[new_size];
//     int new_positions[new_size];
//     merge_components(list1, list2, list, new_positions, size1, size2);
//     for (i=0; i<new_size; i++) printf("%i, ", list[i]);
//     printf("\n");
//     for (i=0; i<(size1 + size2); i++) printf("%i, ", new_positions[i]);
//     printf("\n");
// }