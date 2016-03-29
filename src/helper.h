#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void merge_with_values(int* keys, char* values, int down, int middle, int top,
                       int value_size);
void merge_components(int* keys1, int* keys2, char* values1, char* values2,
                      int size1, int size2, int value_size);
void merge_sort_with_values(int* keys, char* values, int down, int top, int value_size);
