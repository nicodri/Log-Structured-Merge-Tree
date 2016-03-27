#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void merge_with_positions(int list[], int positions[], int down, int middle, int top);
void merge_components(int list1[], int list2[], int list[], int positions[],
                       int size1, int size2);
void merge_sort_with_positions(int list[], int positions[], int down, int top);
void update_values(int* values, int* positions, int value_size, int values_length);
int* build_values(int* values1, int* values2, int* positions, int len1, int len2, int value_size);