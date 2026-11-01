#ifndef MINHEAP_H
#define MINHEAP_H

#include "DynamicArray.h"

template <typename T>
class MinHeap {
private:
    DynamicArray<T> items;

    void swapItems(int i, int j) {
        T temp = items[i];
        items[i] = items[j];
        items[j] = temp;
    }

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

    void insert(const T& value) {
        items.pushBack(value);
        siftUp(items.size() - 1);
    }

    bool peekMin(T& result) const {
        if (isEmpty()) {
            return false;
        }
        result = items[0];
        return true;
    }

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
