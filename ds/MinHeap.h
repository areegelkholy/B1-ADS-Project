// MinHeap.h
// ---------------------------------------------------------------
// A binary min-heap: the SMALLEST element is always at the top.
// This is the project's tree-based (non-linear) data structure.
//
// A heap is a binary tree stored inside an array (no pointers):
//
//   Example tree (value in brackets), stored as the array [1, 3, 2, 7, 4]:
//
//   index 0 [1]  has children  index 1 [3]  and  index 2 [2]
//   index 1 [3]  has children  index 3 [7]  and  index 4 [4]
//
//   parent of i      = (i - 1) / 2
//   left child of i  = 2 * i + 1
//   right child of i = 2 * i + 2
//
// Rule: every parent is smaller than or equal to its children.
//
// WHY WE NEED IT (challenge section: profiler):
//   To report the top-K most executed instructions we keep a min-heap
//   of size K. For each new candidate: if the heap has fewer than K items,
//   insert it. Otherwise compare with the smallest (the top); if the new
//   one is bigger, remove the top and insert the new one. At the end the
//   heap holds exactly the K largest. Cost: O(n log K).
//
// The type T must support the "<" operator (ints do; for our own structs
// we write operator< ourselves, see the test file for an example).
// ---------------------------------------------------------------

#ifndef MINHEAP_H
#define MINHEAP_H

#include "DynamicArray.h"

template <typename T>
class MinHeap {
private:
    DynamicArray<T> items;   // the heap stored as an array

    void swapItems(int i, int j) {
        T temp = items[i];
        items[i] = items[j];
        items[j] = temp;
    }

    // After inserting at the END, move the new item UP until its parent
    // is smaller (or it reaches the top). O(log n).
    void siftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (items[i] < items[parent]) {
                swapItems(i, parent);
                i = parent;
            } else {
                break;   // parent is smaller, heap rule is fine
            }
        }
    }

    // After moving the last item to the TOP, push it DOWN until both
    // children are bigger (or it becomes a leaf). O(log n).
    void siftDown(int i) {
        int n = items.size();
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int smallest = i;                       // who is the smallest of the 3?
            if (left < n && items[left] < items[smallest]) {
                smallest = left;
            }
            if (right < n && items[right] < items[smallest]) {
                smallest = right;
            }
            if (smallest == i) {
                break;                              // already in the right place
            }
            swapItems(i, smallest);
            i = smallest;
        }
    }

public:
    int size() const { return items.size(); }
    bool isEmpty() const { return items.size() == 0; }

    // Add a value. O(log n).
    void insert(const T& value) {
        items.pushBack(value);
        siftUp(items.size() - 1);
    }

    // Look at the smallest value without removing it. O(1).
    bool peekMin(T& result) const {
        if (isEmpty()) {
            return false;
        }
        result = items[0];
        return true;
    }

    // Remove the smallest value and give it back. O(log n).
    bool extractMin(T& result) {
        if (isEmpty()) {
            return false;
        }
        result = items[0];
        // Move the last item to the top, shrink, then fix the order.
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
