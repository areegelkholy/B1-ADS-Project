// Stack.h
// ---------------------------------------------------------------
// A stack (LIFO: last in, first out), built on top of our DynamicArray.
// The TOP of the stack is the LAST element of the array, so push and
// pop are both O(1) (no shifting of elements needed).
//
// WHY WE NEED IT:
//   - Evaluating constant expressions in the assembler (shunting-yard):
//     one stack for the operators and one for the values.
//   - Macro expansion contexts (challenge section).
//
//
// DESIGN DECISIONS:
//   - push/pop return true/false instead of crashing. This lets the
//     caller print its own error message.
//   - A limit of 0 means "no limit".
// ---------------------------------------------------------------

#ifndef STACK_H
#define STACK_H

#include "DynamicArray.h"

template <typename T>
class Stack {
private:
    DynamicArray<T> items; 
    int maxSize;            // 0 = unlimited, otherwise the stack limit

public:

    explicit Stack(int limit = 0) : maxSize(limit) {}

    bool isEmpty() const { return items.size() == 0; }

    // True when a limit exists and we have reached it.
    bool isFull() const { return maxSize > 0 && items.size() >= maxSize; }

    int size() const { return items.size(); }

    // Put a value on top. Returns false on overflow.
    bool push(const T& value) {
        if (isFull()) {
            return false;
        }
        items.pushBack(value);
        return true;
    }

    // Remove the top value and give it back through "result".
    // Returns false if the stack is empty (underflow).
    bool pop(T& result) {
        if (isEmpty()) {
            return false;
        }
        result = items[items.size() - 1];
        items.removeLast();
        return true;
    }

    // Look at the top value without removing it.
    bool peek(T& result) const {
        if (isEmpty()) {
            return false;
        }
        result = items[items.size() - 1];
        return true;
    }

    void clear() { items.clear(); }
};

#endif
