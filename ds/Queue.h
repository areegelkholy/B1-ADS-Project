// Queue.h
// ---------------------------------------------------------------
// A queue (FIFO: first in, first out) built with a singly linked list.
// We remove from the head and add at the tail, and we keep a pointer
// to BOTH ends, so enqueue and dequeue are both O(1).
// (A normal array would need O(n) to remove from the front.)
//
// WHY WE NEED IT:
//   - The simulator's pending-interrupt queue.
//   - The instruction prefetch buffer (limited size, see maxSize).
//   - One queue per stage in the pipeline simulation (challenge).
//   - The output queue of the shunting-yard algorithm (assembler).
//
// The prefetch buffer must be searched ("is address X in the queue?",
// because a SW into the queue forces a flush), so besides the usual
// queue operations we also offer contains() and getAt().
//
// DESIGN DECISIONS:
//   - Optional size limit (0 = unlimited). A full prefetch buffer
//     makes enqueue return false.
//   - Copying a Queue is disabled to avoid two queues owning the same
//     nodes (that would delete the same memory twice).
// ---------------------------------------------------------------

#ifndef QUEUE_H
#define QUEUE_H

template <typename T>
class Queue {
private:
    // One box in the linked list.
    struct Node {
        T value;
        Node* next;
        Node(const T& v) : value(v), next(0) {}
    };

    Node* head;   // front of the queue (next to leave)
    Node* tail;   // back of the queue (last to arrive)
    int count;    // number of elements
    int maxSize;  // 0 = unlimited

public:
    explicit Queue(int limit = 0) : head(0), tail(0), count(0), maxSize(limit) {}

    ~Queue() {
        clear();
    }

    // Copying is not allowed .
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    bool isEmpty() const { return count == 0; }
    bool isFull() const { return maxSize > 0 && count >= maxSize; }
    int size() const { return count; }

    // Add at the back. Returns false if the queue is full.
    bool enqueue(const T& value) {
        if (isFull()) {
            return false;
        }
        Node* newNode = new Node(value);
        if (tail == 0) {
            // queue was empty: the new node is both head and tail
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;   // link after the old tail
            tail = newNode;
        }
        count++;
        return true;
    }

    // Remove from the front and give the value back through "result".
    // Returns false if the queue is empty.
    bool dequeue(T& result) {
        if (head == 0) {
            return false;
        }
        result = head->value;
        Node* oldHead = head;
        head = head->next;
        if (head == 0) {
            tail = 0;               // queue became empty, reset the tail too
        }
        delete oldHead;
        count--;
        return true;
    }

    // Look at the front value without removing it.
    bool peek(T& result) const {
        if (head == 0) {
            return false;
        }
        result = head->value;
        return true;
    }

    // Read the element at position "index" (0 = front) WITHOUT removing it.
    // Returns false if index is out of range.
    // O(n): a linked list has to be walked from the head.
    bool getAt(int index, T& result) const {
        if (index < 0 || index >= count) {
            return false;
        }
        Node* current = head;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        result = current->value;
        return true;
    }

    // Is this value somewhere in the queue? O(n).
    // T must support the == operator (ints do).
    bool contains(const T& value) const {
        Node* current = head;
        while (current != 0) {
            if (current->value == value) {
                return true;
            }
            current = current->next;
        }
        return false;
    }

    // Delete every node.
    void clear() {
        while (head != 0) {
            Node* next = head->next;
            delete head;
            head = next;
        }
        tail = 0;
        count = 0;
    }
};

#endif
