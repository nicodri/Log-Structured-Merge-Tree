// C program to merge k sorted arrays of size n each.
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
 
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

// Declarations
void MinHeap_init(MinHeap *minheap, MinHeapNode* a, int size);
void MinHeapify(MinHeap * minheap, int i);
MinHeapNode getMin(MinHeap *minheap);
void replaceMin(MinHeap *minheap, MinHeapNode x);
void swap(MinHeapNode *x, MinHeapNode *y);
void mergeKArrays(int *output, int *arr, int k, int n);

// to get index of left child of node at index i
int left(int i) { return (2*i + 1); }

// to get index of right child of node at index i
int right(int i) { return (2*i + 2); }
 
// Function acting on a MinHeap
 
// FOLLOWING ARE IMPLEMENTATIONS OF STANDARD MIN HEAP METHODS
// FROM CORMEN BOOK
// Constructor: Builds a heap from a given array a[] of given size
void MinHeap_init(MinHeap *minheap, MinHeapNode* a, int size)
{
    minheap->heap_size = size;
    minheap->harr = a;  // store address of array
    int i = (minheap->heap_size - 1)/2;
    while (i >= 0)
    {
        MinHeapify(minheap, i);
        i--;
    }
}
 
// A recursive method to heapify a subtree with root at given index
// This method assumes that the subtrees are already heapified
void MinHeapify(MinHeap * minheap, int i)
{
    int l = left(i);
    int r = right(i);
    int smallest = i;
    if (l < minheap->heap_size && minheap->harr[l].element < minheap->harr[i].element)
        smallest = l;
    if (r < minheap->heap_size && minheap->harr[r].element < minheap->harr[smallest].element)
        smallest = r;
    if (smallest != i)
    {
        swap(&(minheap->harr[i]), &(minheap->harr[smallest]));
        MinHeapify(minheap, smallest);
    }
}

// to get the root
MinHeapNode getMin(MinHeap *minheap) { return minheap->harr[0]; }

// to replace root with new node x and heapify() new root
void replaceMin(MinHeap *minheap, MinHeapNode x) {
    minheap->harr[0] = x;  MinHeapify(minheap, 0);
}
 
// A utility function to swap two elements
void swap(MinHeapNode *x, MinHeapNode *y)
{
    MinHeapNode temp = *x;  *x = *y;  *y = temp;
}

// This function takes an array of k arrays with n elements.
// arr: [arr1, arr2, ..., arrk]
// All arrays are assumed to be sorted. It merges them together
// and stores them inside output.
void mergeKArrays(int *output, int *arr, int k, int n)
{
    if (output == NULL) output = (int *) malloc(k*n*sizeof(int));
    // Create a min heap with k heap nodes.  Every heap node
    // has first element of an array
    MinHeapNode *harr = (MinHeapNode *) malloc(k*sizeof(MinHeapNode));
    for (int i = 0; i < k; i++)
    {
        harr[i].element = arr[i*n]; // Store the first element
        harr[i].i = i;  // index of array
        harr[i].j = 1;  // Index of next element to be stored from array
    }
    // Create the heap
    MinHeap *hp = (MinHeap*) malloc(sizeof(MinHeap));
    MinHeap_init(hp, harr, k);
 
    // Now one by one get the minimum element from min
    // heap and replace it with next element of its array
    for (int count = 0; count < n*k; count++)
    {
        // Get the minimum element and store it in output
        MinHeapNode root = getMin(hp);
        output[count] = root.element;
 
        // Find the next element that will replace current
        // root of heap. The next element belongs to same
        // array as the current root.
        if (root.j < n)
        {
            root.element = arr[n*root.i + root.j];
            root.j += 1;
        }
        // If root was the last element of its array
        else root.element =  INT_MAX; //INT_MAX is for infinite
 
        // Replace root with next element of array
        replaceMin(hp, root);
    }
}
 
// A utility function to print array elements
void printArray(int* arr, int size)
{
    for (int i=0; i < size; i++) printf("%d, ", arr[i]);
    printf("\n");
}
 
// Driver program to test above functions
int main()
{
    // Change n at the top to change number of elements
    // in an array
    int arr[] =  {2, 6, 12, 34,
                  1, 9, 20, 1000,
                  23, 34, 90, 2000,
                  3, 4, 5, 7};
    int n = 4;
    int k = 4;
 
    int *output = (int*) malloc(n*k*sizeof(int));
    mergeKArrays(output, arr, k, n);
 
    printf("Merged array is \n");
    printArray(output, n*k);
 
    return 0;
}