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
