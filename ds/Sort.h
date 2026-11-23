

// Purpose:
/*   The assembler's listing file ends with the symbol table sorted byname. 
    HashMap gives us the keys in no particular order, so we sort them with this function before printing.

 HOW IT WORKS:
   1) insert every item into a MinHeap          
   2) take the smallest out n times and write
      the results back into the array in order */

#ifndef SORT_H
#define SORT_H

#include "DynamicArray.h"
#include "MinHeap.h"

template <typename T>
void heapSort(DynamicArray<T>& items) {
    MinHeap<T> heap;
    int n = items.size();

    for (int i = 0; i < n; i++) {
        heap.insert(items[i]);
    }

    for (int i = 0; i < n; i++) {
        T smallest;
        heap.extractMin(smallest);
        items[i] = smallest;
    }
}

#endif
