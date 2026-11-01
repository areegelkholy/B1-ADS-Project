// MinHeap.h
// Binary min-heap: the smallest item is always at the top (index 0).
// It is a binary tree stored in an array, so we need no pointers.
//   parent of i = (i - 1) / 2,  left child = 2i + 1,  right child = 2i + 2
// Rule: every parent is <= its children.
// Used for heap sort and for the profiler (top-K most executed instructions).
// T must support the < operator.

#ifndef MINHEAP_H
#define MINHEAP_H

#include "DynamicArray.h"

template <typename T>
class MinHeap {
private:
    DynamicArray<T> items;   // the heap, stored as an array

    void swapItems(int i, int j) {
        T temp = items[i];
        items[i] = items[j];
        items[j] = temp;
    }

    // Move the item at index i up until its parent is smaller. O(log n).
    void siftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (items[i] < items[parent]) {
                swapItems(i, parent);
                i = parent;
            } else {
                break;
            }
        }
    }

    // Move the item at index i down, swapping with its smaller child,
    // until both children are bigger or it is a leaf. O(log n).
    void siftDown(int i) {
        int n = items.size();
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int smallest = i;
            if (left < n && items[left] < items[smallest]) {
                smallest = left;
            }
            if (right < n && items[right] < items[smallest]) {
                smallest = right;
            }
            if (smallest == i) {
                break;
            }
            swapItems(i, smallest);
            i = smallest;
        }
    }

public:
    int size() const { return items.size(); }
    bool isEmpty() const { return items.size() == 0; }

    // Add at the end, then sift up. O(log n).
    void insert(const T& value) {
        items.pushBack(value);
        siftUp(items.size() - 1);
    }

    // Read the smallest without removing it. Returns false if empty. O(1).
    bool peekMin(T& result) const {
        if (isEmpty()) {
            return false;
        }
        result = items[0];
        return true;
    }

    // Remove the smallest. The last item takes its place, then sifts down.
    // Returns false if empty. O(log n).
    bool extractMin(T& result) {
        if (isEmpty()) {
            return false;
        }
        result = items[0];
        items[0] = items[items.size() - 1];
        items.removeLast();
        if (!isEmpty()) {
            siftDown(0);
        }
        return true;
    }

    void clear() { items.clear(); }
};

#endif
