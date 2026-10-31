// DynamicArray.h
// ---------------------------------------------------------------
// A resizable array (our own version of std::vector).
//
// WHY WE NEED IT:
//   - Memory and register file of the simulator (fixed-size use, via fill()).
//   - The storage inside Stack and MinHeap.
//   - Holding the list of source lines / tokens in the assembler.
//
// HOW IT WORKS:
//   We keep a normal C++ array on the heap ("data") that has room for
//   "capacity" items, but only "count" of them are really used.
//   When the array is full and we push one more item, we allocate a new
//   array that is TWICE as big, copy everything over, and delete the old one.
//   Doubling means pushBack is O(1) on average (amortized).
//
// Written as a template so one class works for int, std::string, structs, etc.
// ---------------------------------------------------------------

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdexcept>  // only for std::out_of_range (an error type, not a container)

template <typename T>
class DynamicArray {
private:
    T* data;       // the real array on the heap
    int count;     // how many items are currently stored
    int capacity;  // how many items fit before we must grow

    // Make the array twice as big. Called only when it is full.
    void grow() {
        int newCapacity = capacity * 2;
        T* newData = new T[newCapacity];
        for (int i = 0; i < count; i++) {
            newData[i] = data[i];      // copy old items into the new array
        }
        delete[] data;                 // free the old array
        data = newData;
        capacity = newCapacity;
    }

    // Helper used by the copy constructor and the assignment operator.
    void copyFrom(const DynamicArray& other) {
        capacity = other.capacity;
        count = other.count;
        data = new T[capacity];
        for (int i = 0; i < count; i++) {
            data[i] = other.data[i];
        }
    }

public:
    // Create an empty array. startCapacity must be at least 1.
    explicit DynamicArray(int startCapacity = 1) {
        if (startCapacity < 1) startCapacity = 1;
        capacity = startCapacity;
        count = 0;
        data = new T[capacity];
    }

    // Copy constructor: makes a deep copy so two arrays never share memory.
    DynamicArray(const DynamicArray& other) {
        copyFrom(other);
    }

    // Assignment operator: same idea, but we must free our old memory first.
    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {          // protect against a = a
            delete[] data;
            copyFrom(other);
        }
        return *this;
    }

    // Destructor: give the memory back.
    ~DynamicArray() {
        delete[] data;
    }

    // Add an item at the end. O(1) on average.
    void pushBack(const T& value) {
        if (count == capacity) {
            // "value" could be an item that lives INSIDE this array (for example
            // arr.pushBack(arr[0])). grow() frees the old array, so we must copy
            // the value first, otherwise we would read freed memory.
            T valueCopy = value;
            grow();
            data[count] = valueCopy;
        } else {
            data[count] = value;
        }
        count++;
    }

    // Remove the last item. Does nothing if the array is empty. O(1).
    void removeLast() {
        if (count > 0) {
            count--;
        }
    }

    // Access by position, with bounds checking.  arr[i]
    T& operator[](int index) {
        if (index < 0 || index >= count) {
            throw std::out_of_range("DynamicArray: index out of range");
        }
        return data[index];
    }

    // Same thing for const arrays (needed by const functions in other classes).
    const T& operator[](int index) const {
        if (index < 0 || index >= count) {
            throw std::out_of_range("DynamicArray: index out of range");
        }
        return data[index];
    }

    int size() const { return count; }
    bool isEmpty() const { return count == 0; }

    // Forget all items (the memory is kept for reuse).
    void clear() { count = 0; }

    // Make the array hold exactly n items, all equal to value.
    // The simulator uses this to create its 256-word memory: memory.fill(256, 0);
    void fill(int n, const T& value) {
        clear();
        for (int i = 0; i < n; i++) {
            pushBack(value);
        }
    }
};

#endif
