// Sort.h
// Heap sort using our own MinHeap (no std::sort).
// Used to sort the symbol table keys by name before printing the listing.

#ifndef SORT_H
#define SORT_H

#include "DynamicArray.h"
#include "MinHeap.h"

template <typename T>
void heapSort(DynamicArray<T>& items) {
    MinHeap<T> heap;
    int n = items.size();

    // put everything in the heap
    for (int i = 0; i < n; i++) {
        heap.insert(items[i]);
    }

    // smallest comes out first, so refill from the left
    for (int i = 0; i < n; i++) {
        T smallest;
        heap.extractMin(smallest);
        items[i] = smallest;
    }
}

#endif
